#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	struct emptyControlCacheStruct : public controlCacheStruct
	{
		emptyControlCacheStruct(guiImpl *context, uint32 entity);
		virtual void updateRequestedSize();
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyPress(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyChar(windowClass *win, uint32 key);
	};

	struct panelControlCacheStruct : public controlCacheStruct
	{
		panelControlCacheStruct(guiImpl *context, uint32 entity);
		virtual void updatePositionSize();
		virtual void updateRequestedSize();
		virtual void graphicPrepare();
	};

	struct buttonControlCacheStruct : public controlCacheStruct
	{
		buttonControlCacheStruct(guiImpl *context, uint32 entity);
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
	};

	struct radioButtonControlCacheStruct : public controlCacheStruct
	{
		radioButtonControlCacheStruct(guiImpl *context, uint32 entity);
	};

	struct radioGroupControlCacheStruct : public controlCacheStruct
	{
		radioGroupControlCacheStruct(guiImpl *context, uint32 entity);
		virtual void updatePositionSize();
	};

	struct checkboxControlCacheStruct : public controlCacheStruct
	{
		checkboxControlCacheStruct(guiImpl *context, uint32 entity);
		void click();
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
	};

	struct textboxControlCacheStruct : public controlCacheStruct
	{
		textboxControlCacheStruct(guiImpl *context, uint32 entity);
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
		virtual bool keyChar(windowClass *win, uint32 key);
	};

	struct textareaControlCacheStruct : public controlCacheStruct
	{
		textareaControlCacheStruct(guiImpl *context, uint32 entity);
	};

	namespace
	{
		struct comboboxItemsListCacheStruct;
	}

	struct comboboxControlCacheStruct : public controlCacheStruct
	{
		comboboxItemsListCacheStruct *list;
		comboboxControlCacheStruct(guiImpl *context, uint32 entity);
		const uint32 itemsCount() const;
		uint32 &selected();
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
	};

	struct listboxControlCacheStruct : public controlCacheStruct
	{
		listboxControlCacheStruct(guiImpl *context, uint32 entity);
		const uint32 itemsCount() const;
		virtual void updateRequestedSize();
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
	};

	struct sliderControlCacheStruct : public controlCacheStruct
	{
		sliderControlCacheStruct(guiImpl *context, uint32 entity);
		void updateByMouse();
		float &value();
		virtual void graphicPrepare();
		virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		virtual bool keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers);
	};

	struct scrollbarHorizontalControlCacheStruct : public controlCacheStruct
	{
		scrollbarHorizontalControlCacheStruct(guiImpl *context, uint32 entity);
	};

	struct scrollbarVerticalControlCacheStruct : public controlCacheStruct
	{
		scrollbarVerticalControlCacheStruct(guiImpl *context, uint32 entity);
	};
}
