#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/color.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

#include <typeinfo>

namespace cage
{
	namespace
	{
		configBool renderDebugConfig("cage-engine.gui.renderDebug", false);
		configBool printDebugConfig("cage-engine.gui.printDebug", false);

		struct renderableDebugStruct : public renderableBaseStruct
		{
			struct elementStruct
			{
				vec4 position;
				vec4 color;
			} data;

			virtual void render(guiImpl *impl) override
			{
				guiImpl::graphicsDataStruct &context = impl->graphicsData;
				context.debugShader->bind();
				context.debugShader->uniform(0, data.position);
				context.debugShader->uniform(1, data.color);
				context.debugMesh->bind();
				context.debugMesh->dispatch();
			}
		};

		void printDebug(hierarchyItemStruct *item, uint32 offset)
		{
			item->printDebug(offset);
			if (item->firstChild)
				printDebug(item->firstChild, offset + 1);
			if (item->nextSibling)
				printDebug(item->nextSibling, offset);
		}
	}

	void hierarchyItemStruct::emitDebug() const
	{
		if (!renderDebugConfig)
			return;
		real h = real(hash(ent ? ent->name() : 0)) / real(detail::numeric_limits<uint32>().max());
		emitDebug(renderPos, renderSize, vec4(colorHsvToRgb(vec3(h, 1, 1)), 1));
	}

	void hierarchyItemStruct::emitDebug(vec2 pos, vec2 size, vec4 color) const
	{
		auto *e = impl->emitControl;
		auto *t = e->memory.createObject<renderableDebugStruct>();
		t->setClip(this);
		t->data.position = impl->pointsToNdc(pos, size);
		t->data.color = color;
		e->last->next = t;
		e->last = t;
	}

	void hierarchyItemStruct::printDebug(uint32 offset) const
	{
		string spaces = string().fill(offset * 4);
		CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "HIERARCHY: entity: " + (ent ? ent->name() : 0u) + ", subsided: " + subsidedItem);
		if (item)
		{
			CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "  ITEM: " + typeid(*item).name());
		}
		if (text)
		{
			CAGE_COMPONENT_GUI(text, text, ent);
			CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "  TEXT: '" + text.value + "'");
		}
		CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "  requested size: " + requestedSize);
		CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "  render position: " + renderPos + ", size: " + renderSize);
		CAGE_LOG_CONTINUE(severityEnum::Info, "gui-debug", spaces + "  clip position: " + clipPos + ", size: " + clipSize);
	}

	void printDebug(guiImpl *impl)
	{
		if (!printDebugConfig)
			return;
		CAGE_LOG(severityEnum::Info, "gui-debug", "printing gui hierarchy");
		printDebug(impl->root, 0);
		CAGE_LOG(severityEnum::Info, "gui-debug", "finished gui hierarchy");
		printDebugConfig = false;
	}
}
