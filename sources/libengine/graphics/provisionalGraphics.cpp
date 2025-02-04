#include <set>

#include <cage-core/concurrent.h>
#include <cage-core/string.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/opengl.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/texture.h>
#include <cage-engine/uniformBuffer.h>

namespace cage
{
	namespace
	{
		constexpr uint32 UsageDuration = 3;

		class ProvisionalGraphicsImpl;

		class ProvisionalUniformBufferImpl : public ProvisionalUniformBuffer
		{
		public:
			const String name;
			std::function<void(UniformBuffer *)> init;
			Holder<UniformBuffer> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			uint32 type = m; // 1 = init copied
			uint32 usage = UsageDuration;

			ProvisionalUniformBufferImpl(const String &name) : name(name) {}
		};

		class ProvisionalFrameBufferHandleImpl : public ProvisionalFrameBuffer
		{
		public:
			const String name;
			Holder<FrameBuffer> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			uint32 type = m; // 1 = draw, 2 = read
			uint32 usage = UsageDuration;

			ProvisionalFrameBufferHandleImpl(const String &name) : name(name) {}
		};

		class ProvisionalTextureHandleImpl : public ProvisionalTexture
		{
		public:
			const String name;
			std::function<void(Texture *)> init;
			Holder<Texture> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			uint32 target = m;
			uint32 usage = UsageDuration;

			ProvisionalTextureHandleImpl(const String &name) : name(name) {}
		};

		class ProvisionalGraphicsImpl : public ProvisionalGraphics
		{
		public:
			template<class T>
			struct Container
			{
				Holder<T> acquire(const String &name)
				{
					ScopeLock lock(mutex);
					auto it = data.find(name);
					if (it == data.end())
					{
						Holder<T> v = systemMemory().createHolder<T>(name);
						data.insert(v.share());
						return v;
					}
					return it->share();
				}

				void reset()
				{
					ScopeLock lock(mutex);
					std::erase_if(data,
						[](auto &it)
						{
							if (it->usage == 0)
								return true;
							it->usage--;
							return false;
						});
				}

				void purge()
				{
					ScopeLock lock(mutex);
					for (const auto &it : data)
						it->result.clear(); // make sure the resources are released on the opengl thread, even if there are remaining handles
					data.clear();
				}

				~Container()
				{
					purge(); // make sure the resources are released on the opengl thread
				}

			private:
				Holder<Mutex> mutex = newMutex();

				struct Comparator
				{
					using is_transparent = void;

					bool operator()(const Holder<T> &a, const Holder<T> &b) const { return StringComparatorFast()(a->name, b->name); }

					bool operator()(const Holder<T> &a, const String &b) const { return StringComparatorFast()(a->name, b); }

					bool operator()(const String &a, const Holder<T> &b) const { return StringComparatorFast()(a, b->name); }
				};

				std::set<Holder<T>, Comparator> data;
			};

			Container<ProvisionalUniformBufferImpl> uniformBuffers;
			Container<ProvisionalFrameBufferHandleImpl> frameBuffers;
			Container<ProvisionalTextureHandleImpl> textures;

			Holder<ProvisionalUniformBuffer> uniformBuffer(const String &name, std::function<void(UniformBuffer *)> &&init)
			{
				auto ub = uniformBuffers.acquire(name);
				if (ub->type == m)
				{
					ub->type = 1;
					ub->init = std::move(init);
				}
				return std::move(ub).cast<ProvisionalUniformBuffer>();
			}

			Holder<ProvisionalFrameBuffer> frameBuffer(const String &name, uint32 type)
			{
				auto fb = frameBuffers.acquire(name);
				CAGE_ASSERT(fb->type == m || fb->type == type);
				fb->type = type;
				return std::move(fb).cast<ProvisionalFrameBuffer>();
			}

			Holder<ProvisionalTexture> texture(const String &name, std::function<void(Texture *)> &&init, uint32 target)
			{
				auto t = textures.acquire(name);
				CAGE_ASSERT(t->target == m || t->target == target);
				if (t->target == m)
				{
					t->target = target;
					t->init = std::move(init);
				}
				return std::move(t).cast<ProvisionalTexture>();
			}

			void reset()
			{
				uniformBuffers.reset();
				textures.reset();
				frameBuffers.reset();
			}

			void purge()
			{
				uniformBuffers.purge();
				textures.purge();
				frameBuffers.purge();
			}
		};
	}

	Holder<UniformBuffer> ProvisionalUniformBuffer::resolve()
	{
		ProvisionalUniformBufferImpl *impl = (ProvisionalUniformBufferImpl *)this;
		impl->usage = UsageDuration;
		if (!impl->result)
		{
			impl->result = newUniformBuffer();
			impl->result->setDebugName(impl->name);
			if (impl->init)
				impl->init(+impl->result);
		}
		return impl->result.share();
	}

	Holder<FrameBuffer> ProvisionalFrameBuffer::resolve()
	{
		ProvisionalFrameBufferHandleImpl *impl = (ProvisionalFrameBufferHandleImpl *)this;
		impl->usage = UsageDuration;
		if (!impl->result)
		{
			switch (impl->type)
			{
				case 1:
					impl->result = newFrameBufferDraw();
					break;
				case 2:
					impl->result = newFrameBufferRead();
					break;
			}
			impl->result->setDebugName(impl->name);
		}
		return impl->result.share();
	}

	Holder<Texture> ProvisionalTexture::resolve()
	{
		ProvisionalTextureHandleImpl *impl = (ProvisionalTextureHandleImpl *)this;
		impl->usage = UsageDuration;
		if (!impl->result)
		{
			impl->result = newTexture(impl->target);
			impl->result->setDebugName(impl->name);
			if (impl->init)
				impl->init(+impl->result);
		}
		return impl->result.share();
	}

	Holder<ProvisionalUniformBuffer> ProvisionalGraphics::uniformBuffer(const String &name, std::function<void(UniformBuffer *)> &&init)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->uniformBuffer(name, std::move(init));
	}

	Holder<ProvisionalFrameBuffer> ProvisionalGraphics::frameBufferDraw(const String &name)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->frameBuffer(name, 1);
	}

	Holder<ProvisionalFrameBuffer> ProvisionalGraphics::frameBufferRead(const String &name)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->frameBuffer(name, 2);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture(const String &name, std::function<void(Texture *)> &&init)
	{
		return texture(name, GL_TEXTURE_2D, std::move(init));
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture(const String &name, uint32 target, std::function<void(Texture *)> &&init)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->texture(name, std::move(init), target);
	}

	void ProvisionalGraphics::reset()
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		impl->reset();
	}

	void ProvisionalGraphics::purge()
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		impl->purge();
	}

	Holder<ProvisionalGraphics> newProvisionalGraphics()
	{
		return systemMemory().createImpl<ProvisionalGraphics, ProvisionalGraphicsImpl>();
	}
}
