#include <cage-core/concurrent.h>
#include <cage-core/string.h>

#include <cage-engine/opengl.h>
#include <cage-engine/texture.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/provisionalRenderData.h>

#include <map>

namespace cage
{
	namespace
	{
		class ProvisionalRenderDataImpl;

		class ProvisionalTextureHandleImpl : public ProvisionalTextureHandle
		{
		public:
			Holder<Texture> result;
			ProvisionalRenderDataImpl *impl = nullptr;
			uint32 target = m;
			bool used = true;
		};

		class ProvisionalFrameBufferHandleImpl : public ProvisionalFrameBufferHandle
		{
		public:
			Holder<FrameBuffer> result;
			ProvisionalRenderDataImpl *impl = nullptr;
			uint32 type = m; // 1 = draw, 2 = read
			bool used = true;
		};

		class ProvisionalRenderDataImpl : public ProvisionalRenderData
		{
		public:
			template<class T>
			struct Container
			{
				Holder<Mutex> mutex = newMutex();

				std::map<string, Holder<T>, StringComparatorFast> data;

				Holder<T> acquire(const string &name)
				{
					ScopeLock lock(mutex);
					auto it = data.find(name);
					if (it == data.end())
						return (data[name] = systemArena().createHolder<T>()).share();
					return it->second.share();
				}

				void reset()
				{
					ScopeLock lock(mutex);
					auto it = data.begin();
					while (it != data.end())
					{
						if (it->second->used)
						{
							it->second->used = false;
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
						it.second->result.clear(); // make sure the resources are released on the opengl thread, even if there are remaining handles
					data.clear();
				}

				~Container()
				{
					purge(); // make sure the resources are released on the opengl thread
				}
			};

			Container<ProvisionalTextureHandleImpl> textures;
			Container<ProvisionalFrameBufferHandleImpl> frameBuffers;

			Holder<ProvisionalTextureHandle> texture(const string &name, uint32 target)
			{
				auto t = textures.acquire(name);
				CAGE_ASSERT(t->target == m || t->target == target);
				t->target = target;
				return std::move(t).cast<ProvisionalTextureHandle>();
			}

			Holder<ProvisionalFrameBufferHandle> frameBuffer(const string &name, uint32 type)
			{
				auto fb = frameBuffers.acquire(name);
				CAGE_ASSERT(fb->type == m || fb->type == type);
				fb->type = type;
				return std::move(fb).cast<ProvisionalFrameBufferHandle>();
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

	Holder<Texture> ProvisionalTextureHandle::resolve()
	{
		ProvisionalTextureHandleImpl *impl = (ProvisionalTextureHandleImpl *)this;
		impl->used = true;
		if (!impl->result)
			impl->result = newTexture(impl->target);
		return impl->result.share();
	}

	Holder<FrameBuffer> ProvisionalFrameBufferHandle::resolve()
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
		}
		return impl->result.share();
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::texture(const string &name)
	{
		return texture(name, GL_TEXTURE_2D);
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::texture(const string &name, uint32 target)
	{
		ProvisionalRenderDataImpl *impl = (ProvisionalRenderDataImpl *)this;
		return impl->texture(name, target);
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::texture2dArray(const string &name)
	{
		return texture(name, GL_TEXTURE_2D_ARRAY);
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::textureRectangle(const string &name)
	{
		return texture(name, GL_TEXTURE_RECTANGLE);
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::texture3d(const string &name)
	{
		return texture(name, GL_TEXTURE_3D);
	}

	Holder<ProvisionalTextureHandle> ProvisionalRenderData::textureCube(const string &name)
	{
		return texture(name, GL_TEXTURE_CUBE_MAP);
	}

	Holder<ProvisionalFrameBufferHandle> ProvisionalRenderData::frameBufferDraw(const string &name)
	{
		ProvisionalRenderDataImpl *impl = (ProvisionalRenderDataImpl *)this;
		return impl->frameBuffer(name, 1);
	}

	Holder<ProvisionalFrameBufferHandle> ProvisionalRenderData::frameBufferRead(const string &name)
	{
		ProvisionalRenderDataImpl *impl = (ProvisionalRenderDataImpl *)this;
		return impl->frameBuffer(name, 2);
	}

	void ProvisionalRenderData::reset()
	{
		ProvisionalRenderDataImpl *impl = (ProvisionalRenderDataImpl *)this;
		impl->reset();
	}

	void ProvisionalRenderData::purge()
	{
		ProvisionalRenderDataImpl *impl = (ProvisionalRenderDataImpl *)this;
		impl->purge();
	}

	Holder<ProvisionalRenderData> newProvisionalRenderData()
	{
		return systemArena().createImpl<ProvisionalRenderData, ProvisionalRenderDataImpl>();
	}
}
