#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/memoryBuffer.h>

#include <cage-engine/opengl.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/renderQueue.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class RenderQueueImpl;

		struct CmdBase
		{
			CmdBase *next = nullptr;
			virtual ~CmdBase() = default;
			virtual void dispatch(RenderQueueImpl *impl) = 0;
		};

		// stores memory pointer that needs to be deallocated
		struct MemBlock
		{
			MemBlock *next = nullptr;
			void *ptr = nullptr;
		};

		struct RenderQueueContent
		{
			CmdBase *head = nullptr, *tail = nullptr;
			MemBlock *memHead = nullptr, *memTail = nullptr;

			uint32 commandsCount = 0;
			uint32 drawsCount = 0;
			uint32 primitivesCount = 0;

			UniformBuffer *boundUniformBuffer = nullptr;
			ShaderProgram *boundShaderProgram = nullptr;
			FrameBuffer *boundFrameBuffer = nullptr;
			Texture *boundTexture = nullptr;
			Model *boundModel = nullptr;
			uint32 activeTextureIndex = m;

			Model *lastModel = nullptr;
		};

		class RenderQueueImpl : public RenderQueueContent, public RenderQueue
		{
		public:
			using RenderQueueContent::commandsCount;
			using RenderQueueContent::drawsCount;
			using RenderQueueContent::primitivesCount;

			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			Holder<UniformBuffer> uubObject;
			MemoryBuffer uubStaging;

			RenderQueueImpl()
			{
				initHeads();
			}

			void initHeads()
			{
				struct Cmd : public CmdBase
				{
					void dispatch(RenderQueueImpl *) override
					{}
				};
				head = tail = arena->createObject<Cmd>();
				memHead = memTail = arena->createObject<MemBlock>();
			}

			void reset()
			{
				for (CmdBase *cmd = head; cmd; )
				{
					CmdBase *next = cmd->next;
					arena->destroy<CmdBase>(cmd);
					cmd = next;
				}
				for (MemBlock *mem = memHead; mem; )
				{
					MemBlock *next = mem->next;
					arena->destroy<MemBlock>(mem);
					mem = next;
				}

				*(RenderQueueContent *)this = RenderQueueContent();
				uubStaging.resize(0);
				initHeads();
			}

			void dispatch()
			{
				if (uubStaging.size() > 0)
				{
					uubObject = newUniformBuffer();
					uubObject->writeWhole(uubStaging);
				}

				for (CmdBase *cmd = head; cmd; cmd = cmd->next)
					cmd->dispatch(this);

				uubObject.clear();
			}

			template<class T>
			T &addCmd()
			{
				commandsCount++;
				T *cmd = arena->createObject<T>();
				tail->next = cmd;
				tail = cmd;
				return *cmd;
			}

			template<class T>
			PointerRange<T> copyMem(PointerRange<T> values)
			{
				static_assert(std::is_trivially_copyable_v<T>);
				static_assert(std::is_trivially_destructible_v<T>);
				static_assert(!std::is_pointer_v<T>);
				MemBlock *blck = arena->createObject<MemBlock>();
				memTail->next = blck;
				memTail = blck;
				blck->ptr = arena->allocate(values.size() * sizeof(T), alignof(T));
				detail::memcpy(blck->ptr, values.data(), values.size() * sizeof(T));
				return { (T *)blck->ptr, ((T *)blck->ptr) + values.size() };
			}

			void universalUniformBuffer(uint32 bindingPoint, PointerRange<const char> data)
			{
				const uint32 offset = numeric_cast<uint32>(detail::roundUpTo(uubStaging.size(), UniformBuffer::getAlignmentRequirement()));
				uubStaging.resizeSmart(numeric_cast<uint32>(offset + data.size()));
				detail::memcpy(uubStaging.data() + offset, data.data(), data.size());

				struct Cmd : public CmdBase
				{
					uint32 bindingPoint = 0;
					uint32 offset = 0;
					uint32 size = 0;
					void dispatch(RenderQueueImpl *impl) override
					{
						impl->uubObject->bind(bindingPoint, offset, size);
					}
				};
				RenderQueueImpl *impl = (RenderQueueImpl *)this;
				Cmd &cmd = impl->addCmd<Cmd>();
				cmd.bindingPoint = bindingPoint;
				cmd.offset = offset;
				cmd.size = numeric_cast<uint32>(data.size());
			}

			template<class T>
			void uniformImpl(uint32 name, const T &value)
			{
				struct Cmd : public CmdBase
				{
					T value = {};
					uint32 name = 0;
					void dispatch(RenderQueueImpl *impl) override
					{
						impl->boundShaderProgram->uniform(name, value);
					}
				};
				Cmd &cmd = addCmd<Cmd>();
				cmd.value = value;
				cmd.name = name;
			}

			template<class T>
			void uniformImpl(uint32 name, PointerRange<const T> values)
			{
				struct Cmd : public CmdBase
				{
					PointerRange<const T> values;
					uint32 name = 0;
					void dispatch(RenderQueueImpl *impl) override
					{
						impl->boundShaderProgram->uniform(name, values);
					}
				};
				Cmd &cmd = addCmd<Cmd>();
				cmd.values = copyMem(values);
				cmd.name = name;
			}

			void activeTexture(uint32 t)
			{
				if (t == activeTextureIndex)
					return;
				glActiveTexture(GL_TEXTURE0 + t);
				activeTextureIndex = t;
			}

			void unbindAllTextures()
			{
				GraphicsDebugScope graphicsDebugScope("reset all textures");
				for (uint32 i = 0; i < 16; i++)
				{
					activeTexture(i);
					glBindTexture(GL_TEXTURE_1D, 0);
					glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
					glBindTexture(GL_TEXTURE_3D, 0);
				}
				activeTexture(0);
				boundTexture = nullptr;
			}
		};
	}

	void RenderQueue::universalUniformBuffer(uint32 bindingPoint, PointerRange<const char> data)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->universalUniformBuffer(bindingPoint, data);
	}

	void RenderQueue::bind(Holder<UniformBuffer> &&uniformBuffer, uint32 bindingPoint)
	{
		struct Cmd : public CmdBase
		{
			Holder<UniformBuffer> uniformBuffer;
			uint32 bindingPoint = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				uniformBuffer->bind(bindingPoint);
				impl->boundUniformBuffer = +uniformBuffer;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = templates::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::bind(Holder<UniformBuffer> &&uniformBuffer, uint32 bindingPoint, uint32 offset, uint32 size)
	{
		struct Cmd : public CmdBase
		{
			Holder<UniformBuffer> uniformBuffer;
			uint32 bindingPoint = 0;
			uint32 offset = 0;
			uint32 size = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				uniformBuffer->bind(bindingPoint, offset, size);
				impl->boundUniformBuffer = +uniformBuffer;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = templates::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
		cmd.offset = offset;
		cmd.size = size;
	}

	void RenderQueue::bind(Holder<UniformBuffer> &&uniformBuffer)
	{
		struct Cmd : public CmdBase
		{
			Holder<UniformBuffer> uniformBuffer;
			void dispatch(RenderQueueImpl *impl) override
			{
				uniformBuffer->bind();
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = templates::move(uniformBuffer);
	}

	void RenderQueue::writeWhole(PointerRange<const char> data, uint32 usage)
	{
		struct Cmd : public CmdBase
		{
			PointerRange<const char> data;
			uint32 usage = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundUniformBuffer->writeWhole(data, usage);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.data = impl->copyMem(data);
		cmd.usage = usage;
	}

	void RenderQueue::writeRange(PointerRange<const char> data, uint32 offset)
	{
		struct Cmd : public CmdBase
		{
			PointerRange<const char> data;
			uint32 offset = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundUniformBuffer->writeRange(data, offset);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.data = impl->copyMem(data);
		cmd.offset = offset;
	}

	void RenderQueue::bind(Holder<ShaderProgram> &&shader)
	{
		struct Cmd : public CmdBase
		{
			Holder<ShaderProgram> shaderProgram;
			void dispatch(RenderQueueImpl *impl) override
			{
				shaderProgram->bind();
				impl->boundShaderProgram = +shaderProgram;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.shaderProgram = templates::move(shader);
	}

#define GCHL_GENERATE(TYPE) \
	void RenderQueue::uniform(uint32 name, const TYPE &value) \
	{ \
		RenderQueueImpl *impl = (RenderQueueImpl *)this; \
		impl->uniformImpl<TYPE>(name, value); \
	} \
	void RenderQueue::uniform(uint32 name, PointerRange<const TYPE> values) \
	{ \
		RenderQueueImpl *impl = (RenderQueueImpl *)this; \
		impl->uniformImpl<TYPE>(name, values); \
	}
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(ivec2);
	GCHL_GENERATE(ivec3);
	GCHL_GENERATE(ivec4);
	GCHL_GENERATE(real);
	GCHL_GENERATE(vec2);
	GCHL_GENERATE(vec3);
	GCHL_GENERATE(vec4);
	GCHL_GENERATE(quat);
	GCHL_GENERATE(mat3);
	GCHL_GENERATE(mat4);
#undef GCHL_GENERATE

	void RenderQueue::bind(Holder<FrameBuffer> &&framebuffer)
	{
		struct Cmd : public CmdBase
		{
			Holder<FrameBuffer> framebuffer;
			void dispatch(RenderQueueImpl *impl) override
			{
				framebuffer->bind();
				impl->boundFrameBuffer = +framebuffer;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.framebuffer = templates::move(framebuffer);
	}

	void RenderQueue::depthTexture(Holder<Texture> &&texture)
	{
		struct Cmd : public CmdBase
		{
			Holder<Texture> texture;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundFrameBuffer->depthTexture(+texture);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = templates::move(texture);
	}

	void RenderQueue::colorTexture(uint32 index, Holder<Texture> &&texture, uint32 mipmapLevel)
	{
		struct Cmd : public CmdBase
		{
			Holder<Texture> texture;
			uint32 index = 0;
			uint32 mipmapLevel = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundFrameBuffer->colorTexture(index, +texture, mipmapLevel);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = templates::move(texture);
		cmd.index = index;
		cmd.mipmapLevel = mipmapLevel;
	}

	void RenderQueue::activeAttachments(uint32 mask)
	{
		struct Cmd : public CmdBase
		{
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundFrameBuffer->activeAttachments(mask);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.mask = mask;
	}

	void RenderQueue::clearFrameBuffer()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundFrameBuffer->clear();
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
	}

	void RenderQueue::bind(Holder<Texture> &&texture, uint32 bindingPoint)
	{
		struct Cmd : public CmdBase
		{
			Holder<Texture> texture;
			uint32 bindingPoint = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->activeTexture(bindingPoint);
				texture->bind();
				impl->boundTexture = +texture;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = templates::move(texture);
	}

	void RenderQueue::image2d(uint32 w, uint32 h, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			uint32 w = 0, h = 0, f = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->image2d(w, h, f);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.w = w;
		cmd.h = h;
		cmd.f = internalFormat;
	}

	void RenderQueue::imageCube(uint32 w, uint32 h, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			uint32 w = 0, h = 0, f = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->imageCube(w, h, f);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.w = w;
		cmd.h = h;
		cmd.f = internalFormat;
	}

	void RenderQueue::image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			uint32 w = 0, h = 0, d = 0, f = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->image3d(w, h, d, f);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.w = w;
		cmd.h = h;
		cmd.d = d;
		cmd.f = internalFormat;
	}

	void RenderQueue::filters(uint32 mig, uint32 mag, uint32 aniso)
	{
		struct Cmd : public CmdBase
		{
			uint32 mig = 0, mag = 0, aniso = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->filters(mig, mag, aniso);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.mig = mig;
		cmd.mag = mag;
		cmd.aniso = aniso;
	}

	void RenderQueue::wraps(uint32 s, uint32 t)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, t = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->wraps(s, t);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.t = t;
	}

	void RenderQueue::wraps(uint32 s, uint32 t, uint32 r)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, t = 0, r = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->wraps(s, t, r);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.t = t;
		cmd.r = r;
	}

	void RenderQueue::generateMipmaps()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundTexture->generateMipmaps();
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
	}

	void RenderQueue::unbindAllTextures()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->unbindAllTextures();
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
	}

	void RenderQueue::bind(Holder<Model> &&model)
	{
		struct Cmd : public CmdBase
		{
			Holder<Model> model;
			void dispatch(RenderQueueImpl *impl) override
			{
				model->bind();
				impl->boundModel = +model;
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->lastModel = +model;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.model = templates::move(model);
	}

	void RenderQueue::draw(uint32 instances)
	{
		struct Cmd : public CmdBase
		{
			uint32 instances = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				impl->boundModel->dispatch(instances);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->drawsCount++;
		impl->primitivesCount += instances * impl->lastModel->getPrimitivesCount();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.instances = instances;
	}

	void RenderQueue::viewport(ivec2 origin, ivec2 size)
	{
		struct Cmd : public CmdBase
		{
			ivec2 origin;
			ivec2 size;
			void dispatch(RenderQueueImpl *impl) override
			{
				glViewport(origin[0], origin[1], size[0], size[1]);
				glScissor(origin[0], origin[1], size[0], size[1]);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.origin = origin;
		cmd.size = size;
	}

	void RenderQueue::scissors(ivec2 origin, ivec2 size)
	{
		struct Cmd : public CmdBase
		{
			ivec2 origin;
			ivec2 size;
			void dispatch(RenderQueueImpl *impl) override
			{
				glScissor(origin[0], origin[1], size[0], size[1]);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.origin = origin;
		cmd.size = size;
	}

	void RenderQueue::scissors(bool enable)
	{
		genericEnable(GL_SCISSOR_TEST, enable);
	}

	void RenderQueue::cullingFace(bool front)
	{
		struct Cmd : public CmdBase
		{
			uint32 key = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				glCullFace(key);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.key = front ? GL_FRONT : GL_BACK;
	}

	void RenderQueue::culling(bool enable)
	{
		genericEnable(GL_CULL_FACE, enable);
	}

	void RenderQueue::depthFunc(uint32 func)
	{
		struct Cmd : public CmdBase
		{
			uint32 func = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				glDepthFunc(func);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.func = func;
	}

	void RenderQueue::depthFuncAlways()
	{
		depthFunc(GL_ALWAYS);
	}

	void RenderQueue::depthFuncLessEqual()
	{
		depthFunc(GL_LEQUAL);
	}

	void RenderQueue::depthTest(bool enable)
	{
		genericEnable(GL_DEPTH_TEST, enable);
	}

	void RenderQueue::depthWrite(bool enable)
	{
		struct Cmd : public CmdBase
		{
			bool enable = false;
			void dispatch(RenderQueueImpl *impl) override
			{
				glDepthMask(enable);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.enable = enable;
	}

	void RenderQueue::colorWrite(bool enable)
	{
		struct Cmd : public CmdBase
		{
			bool enable = false;
			void dispatch(RenderQueueImpl *impl) override
			{
				glColorMask(enable, enable, enable, enable);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.enable = enable;
	}

	void RenderQueue::blendFunc(uint32 s, uint32 d)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, d = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				glBlendFunc(s, d);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.d = d;
	}

	void RenderQueue::blendFuncAdditive()
	{
		blendFunc(GL_ONE, GL_ONE);
	}

	void RenderQueue::blendFuncPremultipliedTransparency()
	{
		blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}

	void RenderQueue::blendFuncAlphaTransparency()
	{
		blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void RenderQueue::blending(bool enable)
	{
		genericEnable(GL_BLEND, enable);
	}

	void RenderQueue::clearColor(const vec4 &rgba)
	{
		struct Cmd : public CmdBase
		{
			vec4 rgba;
			void dispatch(RenderQueueImpl *impl) override
			{
				glClearColor(rgba[0].value, rgba[1].value, rgba[2].value, rgba[3].value);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.rgba = rgba;
	}

	void RenderQueue::clear(bool color, bool depth, bool stencil)
	{
		struct Cmd : public CmdBase
		{
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *impl) override
			{
				glClear(mask);
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		if (color) cmd.mask |= GL_COLOR_BUFFER_BIT;
		if (depth) cmd.mask |= GL_DEPTH_BUFFER_BIT;
		if (stencil) cmd.mask |= GL_STENCIL_BUFFER_BIT;
	}

	void RenderQueue::genericEnable(uint32 key, bool enable)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (enable)
		{
			struct Cmd : public CmdBase
			{
				uint32 key = 0;
				void dispatch(RenderQueueImpl *) override
				{
					glEnable(key);
				}
			};
			Cmd &cmd = impl->addCmd<Cmd>();
			cmd.key = key;
		}
		else
		{
			struct Cmd : public CmdBase
			{
				uint32 key = 0;
				void dispatch(RenderQueueImpl *) override
				{
					glDisable(key);
				}
			};
			Cmd &cmd = impl->addCmd<Cmd>();
			cmd.key = key;
		}
	}

	void RenderQueue::pushNamedPass(const string &name)
	{
		struct Cmd : public CmdBase
		{
			string name;
			void dispatch(RenderQueueImpl *impl) override
			{
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.name = name;
	}

	void RenderQueue::popNamedPass()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) override
			{
				glPopDebugGroup();
			}
		};
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
	}

	RenderQueueNamedPassScope RenderQueue::scopedNamedPass(const string &name)
	{
		return RenderQueueNamedPassScope(this, name);
	}

	void RenderQueue::reset()
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->reset();
	}

	void RenderQueue::dispatch()
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->dispatch();
	}

	uint32 RenderQueue::commandsCount() const
	{
		const RenderQueueImpl *impl = (const RenderQueueImpl *)this;
		return impl->commandsCount;
	}

	uint32 RenderQueue::drawsCount() const
	{
		const RenderQueueImpl *impl = (const RenderQueueImpl *)this;
		return impl->drawsCount;
	}

	uint32 RenderQueue::primitivesCount() const
	{
		const RenderQueueImpl *impl = (const RenderQueueImpl *)this;
		return impl->primitivesCount;
	}

	RenderQueueNamedPassScope::RenderQueueNamedPassScope(RenderQueue *queue, const string &name) : queue(queue)
	{
		queue->pushNamedPass(name);
	}

	RenderQueueNamedPassScope::~RenderQueueNamedPassScope()
	{
		queue->popNamedPass();
	}

	Holder<RenderQueue> newRenderQueue()
	{
		return systemArena().createImpl<RenderQueue, RenderQueueImpl>();
	}
}
