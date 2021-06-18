#include <cage-core/concurrent.h>
#include <cage-core/string.h>

#include <cage-engine/opengl.h>
#include <cage-engine/texture.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/provisionalGraphics.h>

#include <set>

namespace cage
{
	namespace
	{
		class ProvisionalGraphicsImpl;

		class ProvisionalUniformBufferImpl : public ProvisionalUniformBuffer
		{
		public:
			const string name;
			Holder<UniformBuffer> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			bool used = true;

			ProvisionalUniformBufferImpl(const string &name) : name(name)
			{}
		};

		class ProvisionalFrameBufferHandleImpl : public ProvisionalFrameBuffer
		{
		public:
			const string name;
			Holder<FrameBuffer> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			uint32 type = m; // 1 = draw, 2 = read
			bool used = true;

			ProvisionalFrameBufferHandleImpl(const string &name) : name(name)
			{}
		};

		class ProvisionalTextureHandleImpl : public ProvisionalTexture
		{
		public:
			const string name;
			Holder<Texture> result;
			ProvisionalGraphicsImpl *impl = nullptr;
			uint32 target = m;
			bool used = true;

			ProvisionalTextureHandleImpl(const string &name) : name(name)
			{}
		};

		class ProvisionalGraphicsImpl : public ProvisionalGraphics
		{
		public:
			template<class T>
			struct Container
			{
				Holder<T> acquire(const string &name)
				{
					ScopeLock lock(mutex);
					auto it = data.find(name);
					if (it == data.end())
					{
						Holder<T> v = systemMemory().createHolder<T>(name);
						data.insert(v.share());
						return std::move(v);
					}
					return it->share();
				}

				void reset()
				{
					ScopeLock lock(mutex);
					auto it = data.begin();
					while (it != data.end())
					{
						if ((*it)->used)
						{
							(*it)->used = false;
							it++;
						}
						else
							it = data.erase(it);
					}
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

					bool operator() (const Holder<T> &a, const Holder<T> &b) const
					{
						return StringComparatorFast()(a->name, b->name);
					}

					bool operator() (const Holder<T> &a, const string &b) const
					{
						return StringComparatorFast()(a->name, b);
					}

					bool operator() (const string &a, const Holder<T> &b) const
					{
						return StringComparatorFast()(a, b->name);
					}
				};

				std::set<Holder<T>, Comparator> data;
			};

			Container<ProvisionalUniformBufferImpl> uniformBuffers;
			Container<ProvisionalFrameBufferHandleImpl> frameBuffers;
			Container<ProvisionalTextureHandleImpl> textures;

			Holder<ProvisionalUniformBuffer> uniformBuffer(const string &name)
			{
				auto ub = uniformBuffers.acquire(name);
				return std::move(ub).cast<ProvisionalUniformBuffer>();
			}

			Holder<ProvisionalFrameBuffer> frameBuffer(const string &name, uint32 type)
			{
				auto fb = frameBuffers.acquire(name);
				CAGE_ASSERT(fb->type == m || fb->type == type);
				fb->type = type;
				return std::move(fb).cast<ProvisionalFrameBuffer>();
			}

			Holder<ProvisionalTexture> texture(const string &name, uint32 target)
			{
				auto t = textures.acquire(name);
				CAGE_ASSERT(t->target == m || t->target == target);
				t->target = target;
				return std::move(t).cast<ProvisionalTexture>();
			}

			void reset()
			{
				textures.reset();
				frameBuffers.reset();
			}

			void purge()
			{
				textures.purge();
				frameBuffers.purge();
			}
		};
	}

	Holder<UniformBuffer> ProvisionalUniformBuffer::resolve()
	{
		ProvisionalUniformBufferImpl *impl = (ProvisionalUniformBufferImpl *)this;
		impl->used = true;
		if (!impl->result)
		{
			impl->result = newUniformBuffer();
			impl->result->setDebugName(impl->name);
		}
		return impl->result.share();
	}

	bool ProvisionalUniformBuffer::ready() const
	{
		const ProvisionalUniformBufferImpl *impl = (const ProvisionalUniformBufferImpl *)this;
		return !!impl->result;
	}

	Holder<FrameBuffer> ProvisionalFrameBuffer::resolve()
	{
		ProvisionalFrameBufferHandleImpl *impl = (ProvisionalFrameBufferHandleImpl *)this;
		impl->used = true;
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

	bool ProvisionalFrameBuffer::ready() const
	{
		const ProvisionalFrameBufferHandleImpl *impl = (const ProvisionalFrameBufferHandleImpl *)this;
		return !!impl->result;
	}

	Holder<Texture> ProvisionalTexture::resolve()
	{
		ProvisionalTextureHandleImpl *impl = (ProvisionalTextureHandleImpl *)this;
		impl->used = true;
		if (!impl->result)
		{
			impl->result = newTexture(impl->target);
			impl->result->setDebugName(impl->name);
		}
		return impl->result.share();
	}

	bool ProvisionalTexture::ready() const
	{
		const ProvisionalTextureHandleImpl *impl = (const ProvisionalTextureHandleImpl *)this;
		return !!impl->result;
	}

	Holder<ProvisionalUniformBuffer> ProvisionalGraphics::uniformBuffer(const string &name)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->uniformBuffer(name);
	}

	Holder<ProvisionalFrameBuffer> ProvisionalGraphics::frameBufferDraw(const string &name)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->frameBuffer(name, 1);
	}

	Holder<ProvisionalFrameBuffer> ProvisionalGraphics::frameBufferRead(const string &name)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->frameBuffer(name, 2);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture(const string &name)
	{
		return texture(name, GL_TEXTURE_2D);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture(const string &name, uint32 target)
	{
		ProvisionalGraphicsImpl *impl = (ProvisionalGraphicsImpl *)this;
		return impl->texture(name, target);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture2dArray(const string &name)
	{
		return texture(name, GL_TEXTURE_2D_ARRAY);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::textureRectangle(const string &name)
	{
		return texture(name, GL_TEXTURE_RECTANGLE);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::texture3d(const string &name)
	{
		return texture(name, GL_TEXTURE_3D);
	}

	Holder<ProvisionalTexture> ProvisionalGraphics::textureCube(const string &name)
	{
		return texture(name, GL_TEXTURE_CUBE_MAP);
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
