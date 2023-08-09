#include <vector>

#include <stb_rect_pack.h>

#include <cage-core/rectPacking.h>

namespace cage
{
	namespace
	{
		stbrp_rect conv(const PackingRect &r)
		{
			stbrp_rect s = {};
			s.id = r.id;
			s.w = r.width;
			s.h = r.height;
			return s;
		}

		PackingRect conv(const stbrp_rect &s)
		{
			CAGE_ASSERT(s.was_packed);
			PackingRect r;
			r.id = s.id;
			r.width = s.w;
			r.height = s.h;
			r.x = s.x;
			r.y = s.y;
			return r;
		}

		class RectPackingImpl : public RectPacking
		{
		public:
			std::vector<PackingRect> data;

			std::vector<stbrp_rect> rs;
			std::vector<stbrp_node> ns;
			stbrp_context ctx;

			bool solve(const RectPackingSolveConfig &config)
			{
				{
					rs.resize(data.size());
					auto a = data.begin();
					for (auto &it : rs)
					{
						it = conv(*a++);
						it.w += config.margin * 2;
						it.h += config.margin * 2;
					}
				}
				ns.resize(config.width);
				stbrp_init_target(&ctx, config.width, config.height, ns.data(), numeric_cast<int>(ns.size()));
				const bool res = stbrp_pack_rects(&ctx, rs.data(), numeric_cast<int>(rs.size()));
				if (res)
				{
					auto a = rs.begin();
					for (auto &it : data)
					{
						it = conv(*a++);
						it.x += config.margin;
						it.y += config.margin;
					}
				}
				return res;
			}
		};
	}

	void RectPacking::resize(uint32 cnt)
	{
		RectPackingImpl *impl = (RectPackingImpl *)this;
		impl->data.resize(cnt);
	}

	PointerRange<PackingRect> RectPacking::data()
	{
		RectPackingImpl *impl = (RectPackingImpl *)this;
		return impl->data;
	}

	PointerRange<const PackingRect> RectPacking::data() const
	{
		const RectPackingImpl *impl = (const RectPackingImpl *)this;
		return impl->data;
	}

	bool RectPacking::solve(const RectPackingSolveConfig &config)
	{
		RectPackingImpl *impl = (RectPackingImpl *)this;
		return impl->solve(config);
	}

	Holder<RectPacking> newRectPacking()
	{
		return systemMemory().createImpl<RectPacking, RectPackingImpl>();
	}
}
