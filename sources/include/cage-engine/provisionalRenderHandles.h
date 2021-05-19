#ifndef guard_provisionalRenderHandles_h_zj6h8sa7eg51f0z5ui
#define guard_provisionalRenderHandles_h_zj6h8sa7eg51f0z5ui

#include "core.h"

namespace cage
{
	namespace privat
	{
		template<class A, class B>
		struct ProvisionalRenderHandle
		{
		public:
			ProvisionalRenderHandle() = default;
			ProvisionalRenderHandle(ProvisionalRenderHandle &&) = default;
			ProvisionalRenderHandle(const ProvisionalRenderHandle &other) : data_(other.data_.share()), type_(other.type_)
			{}
			ProvisionalRenderHandle(const Holder<A> &data) : data_(data.share().template cast<void>()), type_(1)
			{}
			ProvisionalRenderHandle(const Holder<B> &data) : data_(data.share().template cast<void>()), type_(2)
			{}

			ProvisionalRenderHandle &operator = (ProvisionalRenderHandle &&) = default;
			ProvisionalRenderHandle &operator = (const ProvisionalRenderHandle &other)
			{
				data_ = other.data_.share();
				type_ = other.type_;
				return *this;
			}
			ProvisionalRenderHandle &operator = (const Holder<A> &data)
			{
				data_ = data.share().template cast<void>();
				type_ = 1;
				return *this;
			}
			ProvisionalRenderHandle &operator = (const Holder<B> &data)
			{
				data_ = data.share().template cast<void>();
				type_ = 2;
				return *this;
			}

			uint32 type() const
			{
				return type_;
			}

			// requires opengl thread
			Holder<A> resolve() const
			{
				switch (type_)
				{
				case 1:
					return data_.share().template cast<A>();
				case 2:
					return data_.share().template cast<B>()->resolve();
				default:
					return Holder<A>();
				}
			}

			Holder<A> tryResolve() const
			{
				switch (type_)
				{
				case 1:
					return data_.share().template cast<A>();
				case 2:
				{
					Holder<B> tmp = data_.share().template cast<B>();
					if (tmp->ready())
						return tmp->resolve();
					return Holder<A>();
				}
				default:
					return Holder<A>();
				}
			}

			void *pointer() const
			{
				return +data_;
			}

			explicit operator bool() const
			{
				return !!data_;
			}

		private:
			Holder<void> data_;
			uint32 type_ = 0;
		};
	}

	using UniformBufferHandle = privat::ProvisionalRenderHandle<UniformBuffer, ProvisionalUniformBuffer>;
	using FrameBufferHandle = privat::ProvisionalRenderHandle<FrameBuffer, ProvisionalFrameBuffer>;
	using TextureHandle = privat::ProvisionalRenderHandle<Texture, ProvisionalTexture>;
}

#endif // guard_provisionalRenderHandles_h_zj6h8sa7eg51f0z5ui
