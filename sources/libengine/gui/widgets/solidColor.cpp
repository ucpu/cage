#include "../private.h"

namespace cage
{
	namespace
	{
		struct SolidColorImpl;

		struct SolidColorRenderable : public RenderableBase
		{
			Vec4 pos;
			Vec3 rgb;

			SolidColorRenderable(const SolidColorImpl *item);

			~SolidColorRenderable() override
			{
				if (!prepare())
					return;
				RenderQueue *q = impl->activeQueue;
				Holder<ShaderProgram> shader = impl->graphicsData.colorPickerShader[0].share();
				q->bind(shader);
				q->uniform(shader, 0, pos);
				q->uniform(shader, 1, rgb);
				q->draw(impl->graphicsData.imageModel.share());
			}
		};

		struct SolidColorImpl : public WidgetItem
		{
			GuiSolidColorComponent &data;
			Vec3 color; // must keep copy of data.color here because data is not accessible while emitting

			SolidColorImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(SolidColor)) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text && !hierarchy->image);
				color = data.color;
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.solidColor.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.solidColor.margin);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.solidColor.margin);
				SolidColorRenderable t(this);
				t.setClip(hierarchy);
				t.pos = hierarchy->impl->pointsToNdc(p, s);
				t.rgb = color;
			}

			void playHoverSound() override
			{
				// nothing
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				// prevent taking focus
				return false;
			}
		};

		SolidColorRenderable::SolidColorRenderable(const SolidColorImpl *item) : RenderableBase(item->hierarchy->impl) {}
	}

	void SolidColorCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SolidColorImpl>(item).cast<BaseItem>();
	}
}
