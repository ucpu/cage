#include "../private.h"

#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsEncoder.h>

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

				class Command : public GuiRenderCommandBase
				{
				public:
					struct UniData
					{
						Vec4 pos;
						Vec4 colorAndHue; // rgb, hue
					};
					UniData data;
					DrawConfig drw;

					Command(SolidColorRenderable *base)
					{
						GuiRenderImpl *activeQueue = base->impl->activeQueue;

						data.pos = base->pos;
						data.colorAndHue = Vec4(base->rgb, 0);

						drw.shader = +activeQueue->colorPickerShader[0];
						drw.model = +activeQueue->imageModel;
						drw.depthTest = DepthTestEnum::Always;
						drw.depthWrite = false;
					}

					void draw(const GuiRenderConfig &config) override
					{
						const auto ab = config.aggregate->writeStruct(data, 0);
						GraphicsBindingsCreateConfig bind;
						bind.buffers.push_back(ab);
						drw.dynamicOffsets.clear();
						drw.dynamicOffsets.push_back(ab);
						drw.bindings = newGraphicsBindings(config.device, bind);
						config.encoder->draw(drw);
					}
				};

				impl->activeQueue->commands.push_back(impl->activeQueue->memory->createImpl<GuiRenderCommandBase, Command>(this));
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

			void findRequestedSize(Real maxWidth) override
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
