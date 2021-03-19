#include <cage-core/rectPacking.h>

#include <vector>

#include <stb_rect_pack.h>

namespace cage
{
	namespace
	{
		class RectPackingImpl : public RectPacking
		{
		public:
			RectPackingCreateConfig config;
			std::vector<stbrp_node> nodes;
			std::vector<stbrp_rect> rects;

			RectPackingImpl(const RectPackingCreateConfig &config) : config(config)
			{
				rects.reserve(1000);
			}
		};
	}

	void RectPacking::add(uint32 id, uint32 width, uint32 height)
	{
		RectPackingImpl *impl = (RectPackingImpl*)this;
		stbrp_rect r;
		r.id = id;
		r.w = numeric_cast<stbrp_coord>(width + impl->config.margin * 2);
		r.h = numeric_cast<stbrp_coord>(height + impl->config.margin * 2);
		r.x = r.y = 0;
		r.was_packed = 0;
		impl->rects.push_back(r);
	}

	bool RectPacking::solve(uint32 width, uint32 height)
	{
		RectPackingImpl *impl = (RectPackingImpl *)this;
		stbrp_context ctx;
		impl->nodes.resize(width);
		stbrp_init_target(&ctx, width, height, impl->nodes.data(), numeric_cast<int>(impl->nodes.size()));
		return stbrp_pack_rects(&ctx, impl->rects.data(), numeric_cast<int>(impl->rects.size()));
	}

	void RectPacking::get(uint32 index, uint32 &id, uint32 &x, uint32 &y) const
	{
		const RectPackingImpl *impl = (const RectPackingImpl *)this;
		CAGE_ASSERT(index < impl->rects.size());
		auto &r = impl->rects[index];
		CAGE_ASSERT(r.was_packed);
		id = r.id;
		x = r.x + impl->config.margin;
		y = r.y + impl->config.margin;
	}

	uint32 RectPacking::count() const
	{
		const RectPackingImpl *impl = (const RectPackingImpl *)this;
		return numeric_cast<uint32>(impl->rects.size());
	}

	Holder<RectPacking> newRectPacking(const RectPackingCreateConfig &config)
	{
		return detail::systemArena().createImpl<RectPacking, RectPackingImpl>(config);
	}
}
