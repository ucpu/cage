#ifndef guard_provisionalHandles_h_zj6h8sa7eg51f0z5ui
#define guard_provisionalHandles_h_zj6h8sa7eg51f0z5ui

#include "core.h"

namespace cage
{
	namespace privat
	{
		template<class A, class B>
		struct ProvisionalHandle
		{
		public:
			ProvisionalHandle() = default;
			ProvisionalHandle(ProvisionalHandle &&) = default;
			ProvisionalHandle(const ProvisionalHandle &other) : data_(other.data_.share()), type_(other.type_){}
			ProvisionalHandle(const Holder<A> &data) : data_(data.share().template cast<void>()), type_(1) {}
			ProvisionalHandle(const Holder<B> &data) : data_(data.share().template cast<void>()), type_(2) {}

			ProvisionalHandle &operator = (ProvisionalHandle &&) = default;
			ProvisionalHandle &operator = (const ProvisionalHandle &other)
			{
				data_ = other.data_.share();
				type_ = other.type_;
				return *this;
			}
			ProvisionalHandle &operator = (const Holder<A> &data)
			{
				data_ = data.share().template cast<void>();
				type_ = 1;
				return *this;
			}
			ProvisionalHandle &operator = (const Holder<B> &data)
			{
				data_ = data.share().template cast<void>();
				type_ = 2;
				return *this;
			}

			bool operator == (const ProvisionalHandle &other) const
			{
				if (type() != other.type())
					return false;
				return pointer() == other.pointer();
			}

			bool operator != (const ProvisionalHandle &other) const
			{
				return !(*this == other);
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
				case 1: return data_.share().template cast<A>();
				case 2: return data_.share().template cast<B>()->resolve();
				default: return Holder<A>();
				}
			}

			void *pointer() const
			{
				return +data_;
			}

			bool first()
			{
				switch (type_)
				{
				case 2: return data_.share().template cast<B>()->first();
				default: return false;
				}
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

	using UniformBufferHandle = privat::ProvisionalHandle<UniformBuffer, ProvisionalUniformBuffer>;
	using FrameBufferHandle = privat::ProvisionalHandle<FrameBuffer, ProvisionalFrameBuffer>;
	using TextureHandle = privat::ProvisionalHandle<Texture, ProvisionalTexture>;
}

#endif // guard_provisionalHandles_h_zj6h8sa7eg51f0z5ui
