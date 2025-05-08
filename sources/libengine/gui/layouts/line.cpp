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
				CAGE_ASSERT((data.first == LineEdgeModeEnum::Spaced) == (data.last == LineEdgeModeEnum::Spaced));
			}

			void findRequestedSize(Real maxWidth) override
			{
				hierarchy->requestedSize = Vec2();
				for (const auto &c : hierarchy->children)
				{
					c->findRequestedSize(maxWidth);
					hierarchy->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					hierarchy->requestedSize[!data.vertical] = max(hierarchy->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			// returns 0 if c is first child; 1 for inner children; or 2 for last child
			CAGE_FORCE_INLINE uint32 edgeIndex(const Holder<HierarchyItem> &c) const { return &c == &hierarchy->children.front() ? 0 : &c == &hierarchy->children.back() ? 2 : 1; }

			// returns requested width for first/inner/last children
			CAGE_FORCE_INLINE Vec3 edgeRequests() const
			{
				Vec3 r;
				for (const auto &c : hierarchy->children)
					r[edgeIndex(c)] += c->requestedSize[data.vertical];
				return r;
			}

			struct Positioning
			{
				Vec3 sizeFactors = Vec3(1); // first/inner/last
				Real firstOffset;
				Real spacing;
			};

			Positioning calculate(const FinalPosition &update) const
			{
				Positioning p;
				if (update.renderSize[0] < 1e-5 || update.renderSize[1] < 1e-5)
				{
					p.sizeFactors = Vec3();
					return p;
				}
				const Vec3 er = edgeRequests();
				if (data.first == LineEdgeModeEnum::None && data.last == LineEdgeModeEnum::None)
					p.sizeFactors = Vec3(update.renderSize[data.vertical] / max(hierarchy->requestedSize[data.vertical], 1));
				else if (data.first == LineEdgeModeEnum::Spaced)
				{
					CAGE_ASSERT(data.last == LineEdgeModeEnum::Spaced);
					p.spacing = (update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]) / (hierarchy->children.size() - 1);
				}
				else if (data.first == LineEdgeModeEnum::Empty && data.last == LineEdgeModeEnum::Empty)
					p.firstOffset = (update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]) / 2;
				else if (data.first == LineEdgeModeEnum::Flexible && data.last == LineEdgeModeEnum::Flexible)
				{
					const Real f = (update.renderSize[data.vertical] - er[1]) / max(er[0] + er[2], 1);
					p.sizeFactors = Vec3(f, 1, f);
					p.firstOffset = min(0, update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]) / 2;
				}
				else if (data.first == LineEdgeModeEnum::Flexible || (data.last == LineEdgeModeEnum::Flexible && hierarchy->children.size() == 1))
				{
					p.sizeFactors[0] = (update.renderSize[data.vertical] - er[1] - er[2]) / max(er[0], 1);
					p.firstOffset = min(0, update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical]);
				}
				else if (data.last == LineEdgeModeEnum::Flexible)
					p.sizeFactors[2] = (update.renderSize[data.vertical] - er[0] - er[1]) / max(er[2], 1);
				else if (data.first == LineEdgeModeEnum::Empty)
					p.firstOffset = update.renderSize[data.vertical] - hierarchy->requestedSize[data.vertical];
				p.sizeFactors = max(0, p.sizeFactors);
				p.firstOffset = max(0, p.firstOffset);
				p.spacing = max(0, p.spacing);
				Real sum = 0;
				for (const auto &c : hierarchy->children)
					sum += c->requestedSize[data.vertical] * p.sizeFactors[edgeIndex(c)];
				if (sum > update.renderSize[data.vertical])
					p.sizeFactors *= update.renderSize[data.vertical] / sum;
				CAGE_ASSERT(valid(p.sizeFactors) && p.sizeFactors[0].finite() && p.sizeFactors[1].finite() && p.sizeFactors[2].finite());
				return p;
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				const Positioning positioning = calculate(update);
				Real offset = positioning.firstOffset;
				for (const auto &c : hierarchy->children)
				{
					const Real s = c->requestedSize[data.vertical] * positioning.sizeFactors[edgeIndex(c)];
					CAGE_ASSERT(s.valid());
					FinalPosition u(update);
					u.renderSize[data.vertical] = s;
					u.renderPos[data.vertical] += offset;
					if (data.crossAlign.valid())
					{
						u.renderSize[!data.vertical] = min(c->requestedSize[!data.vertical], u.renderSize[!data.vertical]);
						u.renderPos[!data.vertical] += (update.renderSize[!data.vertical] - u.renderSize[!data.vertical]) * data.crossAlign;
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
