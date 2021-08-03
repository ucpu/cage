#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/profiling.h>

#include <cage-engine/opengl.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/provisionalGraphics.h>
#include "private.h"

#include "../blockCollection.h"

#include <vector> // namesStack
#include <array>

namespace cage
{
	namespace
	{
		class RenderQueueImpl;

		struct CmdBase
		{
			CmdBase *next = nullptr;
			virtual ~CmdBase() = default;
			virtual void dispatch(RenderQueueImpl *impl) const = 0;
		};

		enum CemKeys
		{
			CEM_VIEWPORT_X,
			CEM_VIEWPORT_Y,
			CEM_VIEWPORT_W,
			CEM_VIEWPORT_H,
			CEM_SCISSOR_X,
			CEM_SCISSOR_Y,
			CEM_SCISSOR_W,
			CEM_SCISSOR_H,
			CEM_CULLING_FACE,
			CEM_DEPTH_FUNC,
			CEM_DEPTH_WRITE,
			CEM_COLOR_WRITE,
			CEM_BLEND_FUNC_S,
			CEM_BLEND_FUNC_D,

			CEM_TOTAL
		};

		struct ChangeEliminationMachine
		{
			// returns whether the value has changed
			CAGE_FORCE_INLINE bool update(uint32 key, uint32 value)
			{
				const uint32 p = map(key);
				if (p == m)
					return true;
				uint32 &v = values[p];
				if (v == value)
					return false;
				v = value;
				return true;
			}

			ChangeEliminationMachine()
			{
				values.fill(m);
			}

		private:
			std::array<uint32, CEM_TOTAL + 4> values = {};

			CAGE_FORCE_INLINE static constexpr uint32 map(uint32 key)
			{
				if (key < CEM_TOTAL)
					return key;
				switch (key)
				{
				case GL_SCISSOR_TEST: return CEM_TOTAL + 0;
				case GL_CULL_FACE: return CEM_TOTAL + 1;
				case GL_DEPTH_TEST: return CEM_TOTAL + 2;
				case GL_BLEND: return  CEM_TOTAL + 3;
				default: return m;
				}
			}
		};

		// available during dispatch only
		struct RenderQueueDispatchBindings
		{
			Holder<UniformBuffer> uniformBuffer;
			std::array<Holder<Texture>, 16> textures = {};
			Holder<FrameBuffer> frameBuffer;
			Holder<ShaderProgram> shaderProgram;
			Holder<Model> model;
			uint32 activeTextureIndex = m;

			Holder<Texture> &texture()
			{
				CAGE_ASSERT(activeTextureIndex < 16);
				return textures[activeTextureIndex];
			}
		};

		// available during setting up - reset when enqueueing another queue, and by explicit call
		struct RenderQueueSettingBindings
		{
			ChangeEliminationMachine states;
			// pointers used for comparisons only, never to access the objects
			std::array<void *, 16> textures = {};
			void *frameBuffer = nullptr;
			void *shaderProgram = nullptr;
			void *model = nullptr;
			uint32 modelPrimitives = 0;
			uint32 activeTextureIndex = m;

			void *&texture()
			{
				CAGE_ASSERT(activeTextureIndex < 16);
				return textures[activeTextureIndex];
			}
		};

		// available during setting up - reset by explicit call
		struct RenderQueueContent
		{
			BlockCollection<CmdBase *> cmdsAllocs;
			BlockCollection<void *> memAllocs;

			CmdBase *head = nullptr, *tail = nullptr;

			uint32 commandsCount = 0;
			uint32 drawsCount = 0;
			uint32 primitivesCount = 0;

			RenderQueueSettingBindings setting;
		};

		class RenderQueueImpl : public RenderQueueContent, public RenderQueue
		{
		public:
			using RenderQueueContent::commandsCount;
			using RenderQueueContent::drawsCount;
			using RenderQueueContent::primitivesCount;

			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			MemoryBuffer uubStaging;
			UniformBuffer *uubObject = nullptr;
			RenderQueueDispatchBindings *bindings = nullptr;

#ifdef CAGE_DEBUG
			std::vector<StringLiteral> namesStack;
#endif // CAGE_DEBUG
#ifdef CAGE_PROFILING_ENABLED
			std::vector<ProfilingEvent> profilingStack;
#endif // CAGE_PROFILING_ENABLED

			RenderQueueImpl()
			{
				initHeads();
			}

			~RenderQueueImpl()
			{
				resetQueue(); // make sure to properly destroy all the commands, they may hold some assets
			}

			void initHeads()
			{
				struct Cmd : public CmdBase
				{
					void dispatch(RenderQueueImpl *) const override
					{}
				};
				head = tail = arena->createObject<Cmd>();
				cmdsAllocs.push_back(head);
			}

			void resetQueue()
			{
				ProfilingScope profiling("queue reset", "render queue");
				for (const auto &it1 : cmdsAllocs)
					for (CmdBase *it2 : it1)
						arena->destroy<CmdBase>(it2);
				for (const auto &it1 : memAllocs)
					for (void *it2 : it1)
						arena->deallocate(it2);
				*(RenderQueueContent *)this = RenderQueueContent();
				uubStaging.resize(0);
				initHeads();
			}

			void dispatch()
			{
#ifdef CAGE_DEBUG
				namesStack.clear();
#endif // CAGE_DEBUG
#ifdef CAGE_PROFILING_ENABLED
				profilingStack.clear();
#endif // CAGE_PROFILING_ENABLED

				RenderQueueDispatchBindings localBindings; // make sure the bindings are destroyed on the opengl thread
				bindings = &localBindings;

				Holder<UniformBuffer> uub; // make sure the uub is destroyed on the opengl thread
				if (uubStaging.size() > 0)
				{
					ProfilingScope profiling("UUB upload", "render queue");
					uub = newUniformBuffer();
					uub->setDebugName("universal uniform buffer");
					uub->writeWhole(uubStaging);
				}
				uubObject = +uub;

				for (const CmdBase *cmd = head; cmd; cmd = cmd->next)
					cmd->dispatch(this);

				bindings = nullptr;
				uubObject = nullptr;

				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			template<class T>
			T &addCmd()
			{
				commandsCount++;
				T *cmd = arena->createObject<T>();
				cmdsAllocs.push_back(cmd);
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
				void *blck = arena->allocate(values.size() * sizeof(T), alignof(T));
				memAllocs.push_back(blck);
				detail::memcpy(blck, values.data(), values.size() * sizeof(T));
				return { (T *)blck, ((T *)blck) + values.size() };
			}

			UubRange universalUniformBuffer(PointerRange<const char> data, uint32 bindingPoint)
			{
				const uint32 offset = numeric_cast<uint32>(detail::roundUpTo(uubStaging.size(), UniformBuffer::alignmentRequirement()));
				const uint32 size = numeric_cast<uint32>(data.size());
				uubStaging.resizeSmart(numeric_cast<uint32>(offset + size));
				detail::memcpy(uubStaging.data() + offset, data.data(), size);
				UubRange uubRange = { offset, size };
				if (bindingPoint != m)
					bind(uubRange, bindingPoint);
				return uubRange;
			}

			template<class T>
			void uniformImpl(uint32 name, const T &value)
			{
				struct Cmd : public CmdBase
				{
					T value = {};
					uint32 name = 0;
					void dispatch(RenderQueueImpl *impl) const override
					{
						impl->bindings->shaderProgram->uniform(name, value);
					}
				};

				CAGE_ASSERT(setting.shaderProgram);
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
					void dispatch(RenderQueueImpl *impl) const override
					{
						impl->bindings->shaderProgram->uniform(name, values);
					}
				};

				CAGE_ASSERT(setting.shaderProgram);
				Cmd &cmd = addCmd<Cmd>();
				cmd.values = copyMem(values);
				cmd.name = name;
			}

			void activeTexture(uint32 bindingPoint)
			{
				struct Cmd : public CmdBase
				{
					uint32 bindingPoint = 0;
					void dispatch(RenderQueueImpl *impl) const override
					{
						glActiveTexture(GL_TEXTURE0 + bindingPoint);
						impl->bindings->activeTextureIndex = bindingPoint;
					}
				};

				CAGE_ASSERT(bindingPoint < 16);
				if (setting.activeTextureIndex == bindingPoint)
					return;
				setting.activeTextureIndex = bindingPoint;
				Cmd &cmd = addCmd<Cmd>();
				cmd.bindingPoint = bindingPoint;
			}

			void pushNamedScope(StringLiteral name)
			{
#ifdef CAGE_PROFILING_ENABLED
				profilingStack.push_back(profilingEventBegin(string(name), "render queue"));
#endif // CAGE_PROFILING_ENABLED

				struct Cmd : public CmdBase
				{
					StringLiteral name;
					void dispatch(RenderQueueImpl *impl) const override
					{
#ifdef CAGE_DEBUG
						impl->namesStack.push_back(name);
#endif // CAGE_DEBUG
#ifdef CAGE_PROFILING_ENABLED
						impl->profilingStack.push_back(profilingEventBegin(string(name), "render queue"));
#endif // CAGE_PROFILING_ENABLED
						glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
					}
				};

				Cmd &cmd = addCmd<Cmd>();
				cmd.name = name;
			}

			void popNamedScope()
			{
#ifdef CAGE_PROFILING_ENABLED
				profilingEventEnd(profilingStack.back());
				profilingStack.pop_back();
#endif // CAGE_PROFILING_ENABLED

				struct Cmd : public CmdBase
				{
					void dispatch(RenderQueueImpl *impl) const override
					{
#ifdef CAGE_DEBUG
						CAGE_ASSERT(!impl->namesStack.empty());
						impl->namesStack.pop_back();
#endif // CAGE_DEBUG
#ifdef CAGE_PROFILING_ENABLED
						profilingEventEnd(impl->profilingStack.back());
						impl->profilingStack.pop_back();
#endif // CAGE_PROFILING_ENABLED
						glPopDebugGroup();
					}
				};

				addCmd<Cmd>();
			}

		};

		// logical or without short circuiting
		bool or_(bool a, bool b)
		{
			return a || b;
		}

		bool cemUpdate(ChangeEliminationMachine &cem, uint32 key, const ivec4 &value)
		{
			return or_(or_(
				cem.update(key + 0, value[0]),
				cem.update(key + 1, value[1])
			), or_(
				cem.update(key + 2, value[2]),
				cem.update(key + 3, value[3])
			));
		}

		bool cemUpdate(ChangeEliminationMachine &cem, uint32 key, const ivec2 &a, const ivec2 &b)
		{
			return cemUpdate(cem, key, ivec4(a, b));
		}
	}

	UubRange RenderQueue::universalUniformBuffer(PointerRange<const char> data, uint32 bindingPoint)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		return impl->universalUniformBuffer(data, bindingPoint);
	}

	void RenderQueue::bind(UubRange uubRange, uint32 bindingPoint)
	{
		struct Cmd : public CmdBase, public UubRange
		{
			uint32 bindingPoint = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->uubObject->bind(bindingPoint, offset, size);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(uubRange.offset + uubRange.size <= impl->uubStaging.size());
		CAGE_ASSERT((uubRange.offset % UniformBuffer::alignmentRequirement()) == 0);
		Cmd &cmd = impl->addCmd<Cmd>();
		(UubRange&)cmd = uubRange;
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint)
	{
		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			uint32 bindingPoint = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				Holder<UniformBuffer> ub = uniformBuffer.resolve();
				ub->bind(bindingPoint);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(uniformBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint, uint32 offset, uint32 size)
	{
		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			uint32 bindingPoint = 0;
			uint32 offset = 0;
			uint32 size = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				Holder<UniformBuffer> ub = uniformBuffer.resolve();
				ub->bind(bindingPoint, offset, size);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(uniformBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
		cmd.offset = offset;
		cmd.size = size;
	}

	void RenderQueue::bind(UniformBufferHandle uniformBuffer)
	{
		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			void dispatch(RenderQueueImpl *impl) const override
			{
				Holder<UniformBuffer> ub = uniformBuffer.resolve();
				ub->bind();
				impl->bindings->uniformBuffer = std::move(ub);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(uniformBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
	}

	void RenderQueue::writeWhole(PointerRange<const char> data, uint32 usage)
	{
		struct Cmd : public CmdBase
		{
			PointerRange<const char> data;
			uint32 usage = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->uniformBuffer->writeWhole(data, usage);
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->uniformBuffer->writeRange(data, offset);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.data = impl->copyMem(data);
		cmd.offset = offset;
	}

	void RenderQueue::bind(const Holder<ShaderProgram> &shader)
	{
		struct Cmd : public CmdBase
		{
			Holder<ShaderProgram> shaderProgram;
			void dispatch(RenderQueueImpl *impl) const override
			{
				shaderProgram->bind();
				impl->bindings->shaderProgram = shaderProgram.share();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(shader);
		if (impl->setting.shaderProgram == +shader)
			return;
		impl->setting.shaderProgram = +shader;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.shaderProgram = shader.share();
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

	void RenderQueue::bind(FrameBufferHandle frameBuffer)
	{
		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			void dispatch(RenderQueueImpl *impl) const override
			{
				Holder<FrameBuffer> fb = frameBuffer.resolve();
				fb->bind();
				impl->bindings->frameBuffer = std::move(fb);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(frameBuffer);
		if (impl->setting.frameBuffer == frameBuffer.pointer())
			return;
		impl->setting.frameBuffer = frameBuffer.pointer();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.frameBuffer = std::move(frameBuffer);
	}

	void RenderQueue::depthTexture(TextureHandle texture)
	{
		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->frameBuffer->depthTexture(+texture.resolve());
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.frameBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
	}

	void RenderQueue::colorTexture(uint32 index, TextureHandle texture, uint32 mipmapLevel)
	{
		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 index = 0;
			uint32 mipmapLevel = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->frameBuffer->colorTexture(index, +texture.resolve(), mipmapLevel);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.frameBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.index = index;
		cmd.mipmapLevel = mipmapLevel;
	}

	void RenderQueue::activeAttachments(uint32 mask)
	{
		struct Cmd : public CmdBase
		{
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->frameBuffer->activeAttachments(mask);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.frameBuffer);
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.mask = mask;
	}

	void RenderQueue::clearFrameBuffer()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->frameBuffer->clear();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.frameBuffer);
		impl->addCmd<Cmd>();
	}

	void RenderQueue::checkFrameBuffer()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->frameBuffer->checkStatus();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.frameBuffer);
		impl->addCmd<Cmd>();
	}

	void RenderQueue::resetFrameBuffer()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) const override
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				impl->bindings->frameBuffer.clear();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->setting.frameBuffer = nullptr;
		impl->addCmd<Cmd>();
	}

	void RenderQueue::activeTexture(uint32 bindingPoint)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->activeTexture(bindingPoint);
	}

	void RenderQueue::bind(TextureHandle texture, uint32 bindingPoint)
	{
		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			void dispatch(RenderQueueImpl *impl) const override
			{
				Holder<Texture> tex = texture.resolve();
				tex->bind();
				impl->bindings->texture() = std::move(tex);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(texture);
		if (bindingPoint == m)
			bindingPoint = impl->setting.activeTextureIndex == m ? 0 : impl->setting.activeTextureIndex;
		impl->activeTexture(bindingPoint);
		if (impl->setting.texture() == texture.pointer())
			return;
		impl->setting.texture() = texture.pointer();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
	}

	void RenderQueue::image2d(ivec2 resolution, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			ivec2 resolution;
			uint32 internalFormat = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->image2d(resolution, internalFormat);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.resolution = resolution;
		cmd.internalFormat = internalFormat;
	}

	void RenderQueue::imageCube(ivec2 resolution, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			ivec2 resolution;
			uint32 internalFormat = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->imageCube(resolution, internalFormat);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.resolution = resolution;
		cmd.internalFormat = internalFormat;
	}

	void RenderQueue::image3d(ivec3 resolution, uint32 internalFormat)
	{
		struct Cmd : public CmdBase
		{
			ivec3 resolution;
			uint32 internalFormat = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->image3d(resolution, internalFormat);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.resolution = resolution;
		cmd.internalFormat = internalFormat;
	}

	void RenderQueue::filters(uint32 mig, uint32 mag, uint32 aniso)
	{
		struct Cmd : public CmdBase
		{
			uint32 mig = 0, mag = 0, aniso = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->filters(mig, mag, aniso);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->wraps(s, t);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.t = t;
	}

	void RenderQueue::wraps(uint32 s, uint32 t, uint32 r)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, t = 0, r = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->wraps(s, t, r);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.t = t;
		cmd.r = r;
	}

	void RenderQueue::generateMipmaps()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->texture()->generateMipmaps();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.texture());
		Cmd &cmd = impl->addCmd<Cmd>();
	}

	void RenderQueue::resetAllTextures()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *impl) const override
			{
				for (uint32 i = 0; i < 16; i++)
				{
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_1D, 0);
					glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_RECTANGLE, 0);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
					glBindTexture(GL_TEXTURE_3D, 0);
					impl->bindings->textures[i].clear();
				}
				glActiveTexture(GL_TEXTURE0 + 0);
				impl->bindings->activeTextureIndex = 0;
			}
		};

		const auto scopedName = namedScope("reset all textures");
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->setting.textures.fill(nullptr);
		impl->setting.activeTextureIndex = 0;
		impl->addCmd<Cmd>();
	}

	void RenderQueue::bind(const Holder<Model> &model)
	{
		struct Cmd : public CmdBase
		{
			Holder<Model> model;
			void dispatch(RenderQueueImpl *impl) const override
			{
				model->bind();
				impl->bindings->model = model.share();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(model);
		if (impl->setting.model == +model)
			return;
		impl->setting.model = +model;
		impl->setting.modelPrimitives = model->primitivesCount();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.model = model.share();
	}

	void RenderQueue::draw(uint32 instances)
	{
		struct Cmd : public CmdBase
		{
			uint32 instances = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				impl->bindings->model->dispatch(instances);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(impl->setting.model);
		impl->drawsCount++;
		impl->primitivesCount += instances * impl->setting.modelPrimitives;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.instances = instances;
	}

	void RenderQueue::viewport(ivec2 origin, ivec2 size)
	{
		struct Cmd : public CmdBase
		{
			ivec2 origin;
			ivec2 size;
			void dispatch(RenderQueueImpl *impl) const override
			{
				glViewport(origin[0], origin[1], size[0], size[1]);
				glScissor(origin[0], origin[1], size[0], size[1]);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!or_(cemUpdate(impl->setting.states, CEM_VIEWPORT_X, origin, size), cemUpdate(impl->setting.states, CEM_SCISSOR_X, origin, size)))
			return;
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				glScissor(origin[0], origin[1], size[0], size[1]);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!cemUpdate(impl->setting.states, CEM_SCISSOR_X, origin, size))
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.origin = origin;
		cmd.size = size;
	}

	void RenderQueue::scissors(bool enable)
	{
		genericEnable(GL_SCISSOR_TEST, enable);
	}

	void RenderQueue::cullFace(bool front)
	{
		struct Cmd : public CmdBase
		{
			uint32 key = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				glCullFace(key);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!impl->setting.states.update(CEM_CULLING_FACE, front))
			return;
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				glDepthFunc(func);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!impl->setting.states.update(CEM_DEPTH_FUNC, func))
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.func = func;
	}

	void RenderQueue::depthFuncLess()
	{
		depthFunc(GL_LESS);
	}

	void RenderQueue::depthFuncLessEqual()
	{
		depthFunc(GL_LEQUAL);
	}

	void RenderQueue::depthFuncAlways()
	{
		depthFunc(GL_ALWAYS);
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				glDepthMask(enable);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!impl->setting.states.update(CEM_DEPTH_WRITE, enable))
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.enable = enable;
	}

	void RenderQueue::colorWrite(bool enable)
	{
		struct Cmd : public CmdBase
		{
			bool enable = false;
			void dispatch(RenderQueueImpl *impl) const override
			{
				glColorMask(enable, enable, enable, enable);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!impl->setting.states.update(CEM_COLOR_WRITE, enable))
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.enable = enable;
	}

	void RenderQueue::blendFunc(uint32 s, uint32 d)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, d = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				glBlendFunc(s, d);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!or_(impl->setting.states.update(CEM_BLEND_FUNC_S, s), impl->setting.states.update(CEM_BLEND_FUNC_D, d)))
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.s = s;
		cmd.d = d;
	}

	void RenderQueue::blendFuncNone()
	{
		blendFunc(GL_ONE, GL_ZERO);
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
			void dispatch(RenderQueueImpl *impl) const override
			{
				glClearColor(rgba[0].value, rgba[1].value, rgba[2].value, rgba[3].value);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		// todo redundant state removal
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.rgba = rgba;
	}

	void RenderQueue::clear(bool color, bool depth, bool stencil)
	{
		struct Cmd : public CmdBase
		{
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				glClear(mask);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!color && !depth && !stencil)
			return;
		Cmd &cmd = impl->addCmd<Cmd>();
		if (color) cmd.mask |= GL_COLOR_BUFFER_BIT;
		if (depth) cmd.mask |= GL_DEPTH_BUFFER_BIT;
		if (stencil) cmd.mask |= GL_STENCIL_BUFFER_BIT;
	}

	void RenderQueue::genericEnable(uint32 key, bool enable)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		if (!impl->setting.states.update(key, enable))
			return;

		if (enable)
		{
			struct Cmd : public CmdBase
			{
				uint32 key = 0;
				void dispatch(RenderQueueImpl *) const override
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
				void dispatch(RenderQueueImpl *) const override
				{
					glDisable(key);
				}
			};

			Cmd &cmd = impl->addCmd<Cmd>();
			cmd.key = key;
		}
	}

	void RenderQueue::resetAllState()
	{
		const auto scopedName = namedScope("default all state");
		viewport(ivec2(), ivec2());
		scissors(false);
		cullFace(false);
		culling(false);
		depthFuncLess();
		depthTest(false);
		depthWrite(true);
		colorWrite(true);
		blendFuncNone();
		blending(false);
		clearColor(vec4());
	}

	RenderQueueNamedScope RenderQueue::namedScope(StringLiteral name)
	{
		return RenderQueueNamedScope(this, name);
	}

	void RenderQueue::enqueue(const Holder<RenderQueue> &queue)
	{
		struct Cmd : public CmdBase
		{
			Holder<RenderQueue> queue;
			void dispatch(RenderQueueImpl *impl) const override
			{
				queue->dispatch();
				*impl->bindings = RenderQueueDispatchBindings();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(queue);		
		impl->commandsCount += queue->commandsCount();
		impl->drawsCount += queue->drawsCount();
		impl->primitivesCount += queue->primitivesCount();
		impl->setting = RenderQueueSettingBindings();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.queue = queue.share();
	}

	void RenderQueue::customCommand(Delegate<void(void *)> fnc, const Holder<void> &data, bool preservesGlState)
	{
		struct Cmd : public CmdBase
		{
			Delegate<void(void *)> fnc;
			Holder<void> data;
			bool preservesGlState = false;
			void dispatch(RenderQueueImpl *impl) const override
			{
				fnc(+data);
				if (!preservesGlState)
					*impl->bindings = RenderQueueDispatchBindings();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CAGE_ASSERT(fnc);
		if (!preservesGlState)
			impl->setting = RenderQueueSettingBindings();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.fnc = fnc;
		cmd.data = data.share();
		cmd.preservesGlState = preservesGlState;
	}

#ifdef CAGE_DEBUG
	void RenderQueue::checkGlErrorDebug()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *) const override
			{
				try
				{
					cage::checkGlError();
				}
				catch (const GraphicsError &)
				{
					CAGE_LOG(SeverityEnum::Error, "exception", stringizer() + "uncaught opengl error");
				}
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>();
	}
#endif // CAGE_DEBUG

	void RenderQueue::checkGlError()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *) const override
			{
				cage::checkGlError();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>();
	}

	void RenderQueue::resetQueue()
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->resetQueue();
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

	Holder<RenderQueue> newRenderQueue()
	{
		return systemMemory().createImpl<RenderQueue, RenderQueueImpl>();
	}

	RenderQueueNamedScope::RenderQueueNamedScope(RenderQueue *queue, StringLiteral name) : queue(queue)
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)queue;
		impl->pushNamedScope(name);
	}

	RenderQueueNamedScope::~RenderQueueNamedScope()
	{
		RenderQueueImpl *impl = (RenderQueueImpl *)queue;
		impl->popNamedScope();
	}
}
