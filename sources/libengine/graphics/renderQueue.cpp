#include <cage-core/blockContainer.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/profiling.h>
#include <cage-core/memoryAlloca.h>

#include <cage-engine/opengl.h>
#include <cage-engine/model.h>
#include <cage-engine/texture.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/provisionalGraphics.h>

#include <vector> // namesStack
#include <array>

namespace cage
{
	namespace
	{
		class RenderQueueImpl;

		struct CmdBase : private Immovable
		{
			CmdBase *next = nullptr;
			virtual ~CmdBase() = default;
			virtual void dispatch(RenderQueueImpl *impl) const = 0;
		};

		// available during setting up - reset by explicit call
		struct RenderQueueContent
		{
			BlockContainer<CmdBase *> cmdsAllocs;
			BlockContainer<void *> memAllocs;

			CmdBase *setupHead = nullptr, *setupTail = nullptr;
			CmdBase *head = nullptr, *tail = nullptr;

			uint32 commandsCount = 0;
			uint32 drawsCount = 0;
			uint32 primitivesCount = 0;
		};

		class RenderQueueImpl : public RenderQueueContent, public RenderQueue
		{
		public:
			using RenderQueueContent::commandsCount;
			using RenderQueueContent::drawsCount;
			using RenderQueueContent::primitivesCount;

			const String queueName;
			ProvisionalGraphics *const provisionalGraphics = nullptr;
			Holder<MemoryArena> arena = newMemoryAllocatorStream({});
			MemoryBuffer uubStaging;
			UniformBuffer *uubObject = nullptr;

#ifdef CAGE_DEBUG
			std::vector<StringLiteral> namesStack;
#endif // CAGE_DEBUG
#ifdef CAGE_PROFILING_ENABLED
			std::vector<ProfilingEvent> profilingStack;
#endif // CAGE_PROFILING_ENABLED

			RenderQueueImpl(const String &name, ProvisionalGraphics *provisionalGraphics) : queueName(name), provisionalGraphics(provisionalGraphics)
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
				setupHead = setupTail = arena->createObject<Cmd>();
				cmdsAllocs.push_back(setupHead);
				head = tail = arena->createObject<Cmd>();
				cmdsAllocs.push_back(head);
			}

			void resetQueue()
			{
				ProfilingScope profiling("queue reset");
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

				for (const CmdBase *cmd = setupHead; cmd; cmd = cmd->next)
					cmd->dispatch(this);
				
				Holder<UniformBuffer> uub; // make sure the uub is destroyed on the opengl thread
				if (uubStaging.size() > 0)
				{
					ProfilingScope profiling("UUB upload");
					if (provisionalGraphics)
					{
						uub = provisionalGraphics->uniformBuffer(Stringizer() + "UUB_" + queueName)->resolve();
						uub->bind();
						if (uub->size() >= uubStaging.size())
							uub->writeRange(uubStaging, 0);
						else
							uub->writeWhole(uubStaging);
					}
					else
					{
						uub = newUniformBuffer();
						uub->setDebugName("UUB");
						uub->writeWhole(uubStaging);
					}
				}
				uubObject = +uub;

				for (const CmdBase *cmd = head; cmd; cmd = cmd->next)
					cmd->dispatch(this);

				uubObject = nullptr;

				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			// setup commands are run on opengl thread before the universal uniform buffer is dispatched
			template<class T>
			T &addSetup()
			{
				commandsCount++;
				T *cmd = arena->createObject<T>();
				cmdsAllocs.push_back(cmd);
				setupTail->next = cmd;
				setupTail = cmd;
				return *cmd;
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
			void uniformImpl(const Holder<ShaderProgram> &shader, uint32 name, const T &value)
			{
				CAGE_ASSERT(shader);

				struct Cmd : public CmdBase
				{
					Holder<ShaderProgram> shader;
					T value = {};
					uint32 name = 0;
					void dispatch(RenderQueueImpl *) const override
					{
						shader->uniform(name, value);
					}
				};

				Cmd &cmd = addCmd<Cmd>();
				cmd.shader = shader.share();
				cmd.value = value;
				cmd.name = name;
			}

			template<class T>
			void uniformImpl(const Holder<ShaderProgram> &shader, uint32 name, PointerRange<const T> values)
			{
				CAGE_ASSERT(shader);

				struct Cmd : public CmdBase
				{
					Holder<ShaderProgram> shader;
					PointerRange<const T> values;
					uint32 name = 0;
					void dispatch(RenderQueueImpl *) const override
					{
						shader->uniform(name, values);
					}
				};

				Cmd &cmd = addCmd<Cmd>();
				cmd.shader = shader.share();
				cmd.values = copyMem(values);
				cmd.name = name;
			}

			void pushNamedScope(StringLiteral name)
			{
#ifdef CAGE_PROFILING_ENABLED
				profilingStack.push_back(profilingEventBegin(name));
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
						impl->profilingStack.push_back(profilingEventBegin(name));
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
	}

	UubRange RenderQueue::universalUniformBuffer(PointerRange<const char> data, uint32 bindingPoint)
	{
		CAGE_ASSERT(data.size() > 0);
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
		CAGE_ASSERT(uubRange.size <= 16384);
		CAGE_ASSERT(uubRange.offset + uubRange.size <= impl->uubStaging.size());
		CAGE_ASSERT((uubRange.offset % UniformBuffer::alignmentRequirement()) == 0);
		Cmd &cmd = impl->addCmd<Cmd>();
		(UubRange &)cmd = uubRange;
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint)
	{
		CAGE_ASSERT(uniformBuffer);

		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			uint32 bindingPoint = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				Holder<UniformBuffer> ub = uniformBuffer.resolve();
				ub->bind(bindingPoint);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint, uint32 offset, uint32 size)
	{
		CAGE_ASSERT(uniformBuffer);

		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			uint32 bindingPoint = 0;
			uint32 offset = 0;
			uint32 size = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				Holder<UniformBuffer> ub = uniformBuffer.resolve();
				ub->bind(bindingPoint, offset, size);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.bindingPoint = bindingPoint;
		cmd.offset = offset;
		cmd.size = size;
	}

	void RenderQueue::writeWhole(UniformBufferHandle uniformBuffer, PointerRange<const char> data, uint32 usage)
	{
		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			PointerRange<const char> data;
			uint32 usage = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				uniformBuffer.resolve()->writeWhole(data, usage);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.data = impl->copyMem(data);
		cmd.usage = usage;
	}

	void RenderQueue::writeRange(UniformBufferHandle uniformBuffer, PointerRange<const char> data, uint32 offset)
	{
		struct Cmd : public CmdBase
		{
			UniformBufferHandle uniformBuffer;
			PointerRange<const char> data;
			uint32 offset = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				uniformBuffer.resolve()->writeRange(data, offset);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.uniformBuffer = std::move(uniformBuffer);
		cmd.data = impl->copyMem(data);
		cmd.offset = offset;
	}

	void RenderQueue::bind(const Holder<ShaderProgram> &shader)
	{
		CAGE_ASSERT(shader);

		struct Cmd : public CmdBase
		{
			Holder<ShaderProgram> shaderProgram;
			void dispatch(RenderQueueImpl *) const override
			{
				shaderProgram->bind();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().shaderProgram = shader.share();
	}

#define GCHL_GENERATE(TYPE) \
	void RenderQueue::uniform(const Holder<ShaderProgram> &shader, uint32 name, const TYPE &value) \
	{ \
		RenderQueueImpl *impl = (RenderQueueImpl *)this; \
		impl->uniformImpl<TYPE>(shader, name, value); \
	} \
	void RenderQueue::uniform(const Holder<ShaderProgram> &shader, uint32 name, PointerRange<const TYPE> values) \
	{ \
		RenderQueueImpl *impl = (RenderQueueImpl *)this; \
		impl->uniformImpl<TYPE>(shader, name, values); \
	}
	GCHL_GENERATE(sint32);
	GCHL_GENERATE(uint32);
	GCHL_GENERATE(Vec2i);
	GCHL_GENERATE(Vec3i);
	GCHL_GENERATE(Vec4i);
	GCHL_GENERATE(Real);
	GCHL_GENERATE(Vec2);
	GCHL_GENERATE(Vec3);
	GCHL_GENERATE(Vec4);
	GCHL_GENERATE(Quat);
	GCHL_GENERATE(Mat3);
	GCHL_GENERATE(Mat4);
#undef GCHL_GENERATE

	void RenderQueue::bind(FrameBufferHandle frameBuffer)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			void dispatch(RenderQueueImpl *impl) const override
			{
				frameBuffer.resolve()->bind();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().frameBuffer = std::move(frameBuffer);
	}

	void RenderQueue::depthTexture(FrameBufferHandle frameBuffer, TextureHandle texture)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			TextureHandle texture;
			void dispatch(RenderQueueImpl *impl) const override
			{
				frameBuffer.resolve()->depthTexture(+texture.resolve());
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.frameBuffer = std::move(frameBuffer);
		cmd.texture = std::move(texture);
	}

	void RenderQueue::colorTexture(FrameBufferHandle frameBuffer, uint32 index, TextureHandle texture, uint32 mipmapLevel)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			TextureHandle texture;
			uint32 index = 0;
			uint32 mipmapLevel = 0;
			void dispatch(RenderQueueImpl *impl) const override
			{
				frameBuffer.resolve()->colorTexture(index, +texture.resolve(), mipmapLevel);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.frameBuffer = std::move(frameBuffer);
		cmd.texture = std::move(texture);
		cmd.index = index;
		cmd.mipmapLevel = mipmapLevel;
	}

	void RenderQueue::activeAttachments(FrameBufferHandle frameBuffer, uint32 mask)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				frameBuffer.resolve()->activeAttachments(mask);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.frameBuffer = std::move(frameBuffer);
		cmd.mask = mask;
	}

	void RenderQueue::clearFrameBuffer(FrameBufferHandle frameBuffer)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			void dispatch(RenderQueueImpl *) const override
			{
				frameBuffer.resolve()->clear();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().frameBuffer = std::move(frameBuffer);
	}

	void RenderQueue::checkFrameBuffer(FrameBufferHandle frameBuffer)
	{
		CAGE_ASSERT(frameBuffer);

		struct Cmd : public CmdBase
		{
			FrameBufferHandle frameBuffer;
			void dispatch(RenderQueueImpl *impl) const override
			{
				frameBuffer.resolve()->checkStatus();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().frameBuffer = std::move(frameBuffer);
	}

	void RenderQueue::resetFrameBuffer()
	{
		struct Cmd : public CmdBase
		{
			void dispatch(RenderQueueImpl *) const override
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>();
	}

	void RenderQueue::bind(TextureHandle texture, uint32 bindingPoint)
	{
		CAGE_ASSERT(texture);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 bindingPoint = m;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->bind(bindingPoint);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.bindingPoint = bindingPoint;
	}

	void RenderQueue::image2d(TextureHandle texture, Vec2i resolution, uint32 mipmapLevels, uint32 internalFormat)
	{
		CAGE_ASSERT(texture);
		CAGE_ASSERT(mipmapLevels > 0);
		CAGE_ASSERT(internalFormat != 0);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			Vec2i resolution;
			uint32 mipmapLevels = 0;
			uint32 internalFormat = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->initialize(resolution, mipmapLevels, internalFormat);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.resolution = resolution;
		cmd.mipmapLevels = mipmapLevels;
		cmd.internalFormat = internalFormat;
	}

	void RenderQueue::image3d(TextureHandle texture, Vec3i resolution, uint32 mipmapLevels, uint32 internalFormat)
	{
		CAGE_ASSERT(texture);
		CAGE_ASSERT(mipmapLevels > 0);
		CAGE_ASSERT(internalFormat != 0);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			Vec3i resolution;
			uint32 mipmapLevels = 0;
			uint32 internalFormat = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->initialize(resolution, mipmapLevels, internalFormat);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.resolution = resolution;
		cmd.mipmapLevels = mipmapLevels;
		cmd.internalFormat = internalFormat;
	}

	void RenderQueue::filters(TextureHandle texture, uint32 mig, uint32 mag, uint32 aniso)
	{
		CAGE_ASSERT(texture);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 mig = 0, mag = 0, aniso = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->filters(mig, mag, aniso);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.mig = mig;
		cmd.mag = mag;
		cmd.aniso = aniso;
	}

	void RenderQueue::wraps(TextureHandle texture, uint32 s, uint32 t)
	{
		CAGE_ASSERT(texture);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 s = 0, t = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->wraps(s, t);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.s = s;
		cmd.t = t;
	}

	void RenderQueue::wraps(TextureHandle texture, uint32 s, uint32 t, uint32 r)
	{
		CAGE_ASSERT(texture);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 s = 0, t = 0, r = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->wraps(s, t, r);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = std::move(texture);
		cmd.s = s;
		cmd.t = t;
		cmd.r = r;
	}

	void RenderQueue::generateMipmaps(TextureHandle texture)
	{
		CAGE_ASSERT(texture);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->generateMipmaps();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().texture = std::move(texture);
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
				}
				glActiveTexture(GL_TEXTURE0 + 0);
			}
		};

		const auto scopedName = namedScope("reset all textures");
		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>();
	}

	void RenderQueue::bindImage(TextureHandle texture, uint32 bindingPoint, bool read, bool write)
	{
		CAGE_ASSERT(read || write);

		struct Cmd : public CmdBase
		{
			TextureHandle texture;
			uint32 bindingPoint = 0;
			bool read = false;
			bool write = false;
			void dispatch(RenderQueueImpl *) const override
			{
				texture.resolve()->bindImage(bindingPoint, read, write);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.texture = texture;
		cmd.bindingPoint = bindingPoint;
		cmd.read = read;
		cmd.write = write;
	}

	void RenderQueue::bindlessUniform(Holder<PointerRange<TextureHandle>> bindlessHandles, uint32 bindingPoint, bool makeResident)
	{
		struct CmdSetup : public CmdBase
		{
			Holder<PointerRange<TextureHandle>> bindlessHandles;
			UubRange range;

			void dispatch(RenderQueueImpl *impl) const override
			{
				uint64 *arr = (uint64 *)CAGE_ALLOCA(sizeof(uint64) * bindlessHandles.size());
				uint64 *arrit = arr;
				for (auto &it : bindlessHandles)
				{
					if (!it)
					{
						*arrit++ = 0;
						continue;
					}
					Holder<Texture> tex = it.resolve();
					BindlessHandle hnd = tex->bindlessHandle();
					*arrit++ = hnd.handle;
				}
				const_cast<CmdSetup *>(this)->range = impl->universalUniformArray<uint64>(PointerRange<uint64>(arr, arr + bindlessHandles.size()));
			}
		};

		struct CmdDispatch : public CmdBase
		{
			const CmdSetup *setup = nullptr;
			uint32 bindingPoint = m;
			bool makeResident = false;

			void dispatch(RenderQueueImpl *impl) const override
			{
				if (makeResident)
				{
					for (auto &it : setup->bindlessHandles)
					{
						if (!it)
							continue;
						Holder<Texture> tex = it.resolve();
						tex->makeResident(true);
					}
				}

				impl->uubObject->bind(bindingPoint, setup->range.offset, setup->range.size);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		CmdSetup &setup = impl->addSetup<CmdSetup>();
		setup.bindlessHandles = std::move(bindlessHandles);
		CmdDispatch &cmd = impl->addCmd<CmdDispatch>();
		cmd.setup = &setup;
		cmd.bindingPoint = bindingPoint;
		cmd.makeResident = makeResident;
	}

	void RenderQueue::bindlessResident(Holder<PointerRange<TextureHandle>> bindlessHandles, bool resident)
	{
		struct Cmd : public CmdBase
		{
			Holder<PointerRange<TextureHandle>> bindlessHandles;
			bool resident = false;

			void dispatch(RenderQueueImpl *) const override
			{
				for (auto &it : bindlessHandles)
				{
					Holder<Texture> tex = it.resolve();
					tex->makeResident(resident);
				}
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.bindlessHandles = std::move(bindlessHandles);
		cmd.resident = resident;
	}

	void RenderQueue::draw(const Holder<Model> &model, uint32 instances)
	{
		CAGE_ASSERT(model);

		struct Cmd : public CmdBase
		{
			Holder<Model> model;
			uint32 instances = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				model->bind();
				model->dispatch(instances);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->drawsCount++;
		impl->primitivesCount += instances * model->primitivesCount();
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.model = model.share();
		cmd.instances = instances;
	}

	void RenderQueue::compute(const Holder<ShaderProgram> &shader, const Vec3i &groupsCounts)
	{
		struct Cmd : public CmdBase
		{
			Holder<ShaderProgram> shader;
			Vec3i groupsCounts;
			void dispatch(RenderQueueImpl *) const override
			{
				shader->bind();
				shader->compute(groupsCounts);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->drawsCount++;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.shader = shader.share();
		cmd.groupsCounts = groupsCounts;
	}

	void RenderQueue::memoryBarrier(uint32 bits)
	{
		struct Cmd : public CmdBase
		{
			uint32 bits = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				glMemoryBarrier(bits);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		Cmd &cmd = impl->addCmd<Cmd>();
		cmd.bits = bits;
	}

	void RenderQueue::viewport(Vec2i origin, Vec2i size)
	{
		struct Cmd : public CmdBase
		{
			Vec2i origin;
			Vec2i size;
			void dispatch(RenderQueueImpl *) const override
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

	void RenderQueue::scissors(Vec2i origin, Vec2i size)
	{
		struct Cmd : public CmdBase
		{
			Vec2i origin;
			Vec2i size;
			void dispatch(RenderQueueImpl *) const override
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

	void RenderQueue::cullFace(bool front)
	{
		struct Cmd : public CmdBase
		{
			uint32 key = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				glCullFace(key);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().key = front ? GL_FRONT : GL_BACK;
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
			void dispatch(RenderQueueImpl *) const override
			{
				glDepthFunc(func);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().func = func;
	}

	void RenderQueue::depthFuncLess()
	{
		depthFunc(GL_LESS);
	}

	void RenderQueue::depthFuncLessEqual()
	{
		depthFunc(GL_LEQUAL);
	}

	void RenderQueue::depthFuncEqual()
	{
		depthFunc(GL_EQUAL);
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
			void dispatch(RenderQueueImpl *) const override
			{
				glDepthMask(enable);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().enable = enable;
	}

	void RenderQueue::colorWrite(bool enable)
	{
		struct Cmd : public CmdBase
		{
			bool enable = false;
			void dispatch(RenderQueueImpl *) const override
			{
				glColorMask(enable, enable, enable, enable);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().enable = enable;
	}

	void RenderQueue::blendFunc(uint32 s, uint32 d)
	{
		struct Cmd : public CmdBase
		{
			uint32 s = 0, d = 0;
			void dispatch(RenderQueueImpl *) const override
			{
				glBlendFunc(s, d);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
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

	void RenderQueue::clearColor(const Vec4 &rgba)
	{
		struct Cmd : public CmdBase
		{
			Vec4 rgba;
			void dispatch(RenderQueueImpl *) const override
			{
				glClearColor(rgba[0].value, rgba[1].value, rgba[2].value, rgba[3].value);
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->addCmd<Cmd>().rgba = rgba;
	}

	void RenderQueue::clear(bool color, bool depth, bool stencil)
	{
		struct Cmd : public CmdBase
		{
			uint32 mask = 0;
			void dispatch(RenderQueueImpl *) const override
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
				void dispatch(RenderQueueImpl *) const override
				{
					glEnable(key);
				}
			};
			impl->addCmd<Cmd>().key = key;
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
			impl->addCmd<Cmd>().key = key;
		}
	}

	void RenderQueue::resetAllState()
	{
		const auto scopedName = namedScope("default all state");
		viewport(Vec2i(), Vec2i());
		scissors(false);
		cullFace(false);
		culling(false);
		depthFuncLess();
		depthTest(false);
		depthWrite(true);
		colorWrite(true);
		blendFuncNone();
		blending(false);
		clearColor(Vec4());
	}

	RenderQueueNamedScope RenderQueue::namedScope(StringLiteral name)
	{
		return RenderQueueNamedScope(this, name);
	}

	void RenderQueue::enqueue(Holder<RenderQueue> queue)
	{
		CAGE_ASSERT(queue);

		struct Cmd : public CmdBase
		{
			Holder<RenderQueue> queue;
			void dispatch(RenderQueueImpl *impl) const override
			{
				queue->dispatch();
			}
		};

		RenderQueueImpl *impl = (RenderQueueImpl *)this;
		impl->commandsCount += queue->commandsCount();
		impl->drawsCount += queue->drawsCount();
		impl->primitivesCount += queue->primitivesCount();
		impl->addCmd<Cmd>().queue = queue.share();
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
					CAGE_LOG(SeverityEnum::Error, "exception", Stringizer() + "uncaught opengl error");
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

	Holder<RenderQueue> newRenderQueue(const String &name, ProvisionalGraphics *provisionalGraphics)
	{
		return systemMemory().createImpl<RenderQueue, RenderQueueImpl>(name, provisionalGraphics);
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
