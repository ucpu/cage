#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/string.h>

#include "../private.h"

#include <typeinfo>

namespace cage
{
	namespace
	{
		ConfigBool renderDebugConfig("cage/gui/renderHierarchy", false);
		ConfigBool printDebugConfig("cage/gui/logHierarchy", false);

		struct RenderableDebug : public RenderableBase
		{
			struct Element
			{
				vec4 position;
				vec4 color;
			} data;

			virtual void render(GuiImpl *impl) override
			{
				GuiImpl::GraphicsData &context = impl->graphicsData;
				context.debugShader->bind();
				context.debugShader->uniform(0, data.position);
				context.debugShader->uniform(1, data.color);
				context.debugMesh->bind();
				context.debugMesh->dispatch();
			}
		};

		void printDebug(HierarchyItem *item, uint32 offset)
		{
			item->printDebug(offset);
			if (item->firstChild)
				printDebug(item->firstChild, offset + 1);
			if (item->nextSibling)
				printDebug(item->nextSibling, offset);
		}
	}

	void HierarchyItem::emitDebug() const
	{
		if (!renderDebugConfig)
			return;
		real h = real(hash(ent ? ent->name() : 0)) / real(std::numeric_limits<uint32>().max());
		emitDebug(renderPos, renderSize, vec4(colorHsvToRgb(vec3(h, 1, 1)), 1));
	}

	void HierarchyItem::emitDebug(vec2 pos, vec2 size, vec4 color) const
	{
		auto *e = impl->emitControl;
		auto *t = e->memory.createObject<RenderableDebug>();
		t->setClip(this);
		t->data.position = impl->pointsToNdc(pos, size);
		t->data.color = color;
		e->last->next = t;
		e->last = t;
	}

	void HierarchyItem::printDebug(uint32 offset) const
	{
		string spaces = string().fill(offset * 4);
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "HIERARCHY: entity: " + (ent ? ent->name() : 0u) + ", subsided: " + subsidedItem);
		if (item)
		{
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "  ITEM: " + typeid(*item).name());
		}
		if (text)
		{
			CAGE_COMPONENT_GUI(Text, text, ent);
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "  TEXT: '" + text.value + "'");
		}
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "  requested size: " + requestedSize);
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "  render position: " + renderPos + ", size: " + renderSize);
		CAGE_LOG_CONTINUE(SeverityEnum::Info, "gui-debug", stringizer() + spaces + "  clip position: " + clipPos + ", size: " + clipSize);
	}

	void printDebug(GuiImpl *impl)
	{
		if (!printDebugConfig)
			return;
		CAGE_LOG(SeverityEnum::Info, "gui-debug", "printing gui hierarchy");
		printDebug(impl->root, 0);
		CAGE_LOG(SeverityEnum::Info, "gui-debug", "finished gui hierarchy");
		printDebugConfig = false;
	}
}
