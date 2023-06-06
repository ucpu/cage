#include "../private.h"

namespace cage
{
	namespace
	{
		struct LineImpl : public LayoutItem
		{
			const GuiLayoutLineComponent &data;

			LineImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(LayoutLine)) {}

			void initialize() override
			{
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT((data.begin == LineEdgeModeEnum::Spaced) == (data.end == LineEdgeModeEnum::Spaced));
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = Vec2();
				for (const auto &c : hierarchy->children)
				{
					c->findRequestedSize();
					hierarchy->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					hierarchy->requestedSize[!data.vertical] = max(hierarchy->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			CAGE_FORCE_INLINE uint32 edgeIndex(const Holder<HierarchyItem> &c) const { return &c == &hierarchy->children.front() ? 0 : &c == &hierarchy->children.back() ? 2 : 1; }

			CAGE_FORCE_INLINE Vec3 edgeRequests() const
			{
				Vec3 r;
				for (const auto &c : hierarchy->children)
					r[edgeIndex(c)] += c->requestedSize[data.vertical];
				return r;
			}

			struct Positioning
			{
				Vec3 sizeFactors = Vec3(1); // first, center, last
				Real firstOffset;
				Real spacing;
			};

			CAGE_FORCE_INLINE Positioning calculate(const FinalPosition &update) const
			{
				Positioning p;
				const Vec3 er = edgeRequests();
				if (data.begin == LineEdgeModeEnum::None && data.end == LineEdgeModeEnum::None)
					p.sizeFactors = Vec3(update.renderSize[data.vertical] / hierarchy->requestedSize[data.vertical]);
				else if (data.begin == LineEdgeModeEnum::Spaced)
				{
					CAGE_ASSERT(data.end == LineEdgeModeEnum::Spaced);
					p.spacing = max(0, update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]) / (hierarchy->children.size() - 1);
				}
				else if (data.begin == LineEdgeModeEnum::Empty && data.end == LineEdgeModeEnum::Empty)
					p.firstOffset = max(0, update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]) / 2;
				else if (data.begin == LineEdgeModeEnum::Flexible && data.end == LineEdgeModeEnum::Flexible)
				{
					const Real f = max(0, update.renderSize[data.vertical] - er[1]) / (er[0] + er[2]);
					p.sizeFactors = Vec3(f, 1, f);
				}
				else if (data.begin == LineEdgeModeEnum::Flexible || (data.end == LineEdgeModeEnum::Flexible && hierarchy->children.size() == 1))
					p.sizeFactors[0] = max(0, update.renderSize[data.vertical] - er[1] - er[2]) / er[0];
				else if (data.end == LineEdgeModeEnum::Flexible)
					p.sizeFactors[2] = max(0, update.renderSize[data.vertical] - er[0] - er[1]) / er[2];
				else if (data.begin == LineEdgeModeEnum::Empty)
					p.firstOffset = max(0, update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]);
				return p;
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				const Positioning positioning = calculate(update);
				Real offset = positioning.firstOffset;
				for (const auto &c : hierarchy->children)
				{
					Real s = c->requestedSize[data.vertical] * positioning.sizeFactors[edgeIndex(c)];
					if (!valid(s))
						s = 0;
					FinalPosition u(update);
					u.renderSize[data.vertical] = s;
					u.renderPos[data.vertical] += offset;
					if (data.crossAlign.valid())
					{
						u.renderSize[!data.vertical] = c->requestedSize[!data.vertical];
						u.renderPos[!data.vertical] += (update.renderSize[!data.vertical] - c->requestedSize[!data.vertical]) * data.crossAlign;
					}
					c->findFinalPosition(u);
					offset += s + positioning.spacing;
				}
			}
		};
	}

	void LayoutLineCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<LineImpl>(item).cast<BaseItem>();
	}
}
