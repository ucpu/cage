#include <cage-engine/inputs.h>

namespace cage
{
#define GENERALIZER(CLASS) \
	GenericInput g; \
	g.data = in; \
	g.type = InputClassEnum::CLASS; \
	return dispatcher.dispatch(g);

	bool InputsGeneralizer::focusGain(InputWindow in)
	{
		GENERALIZER(FocusGain);
	}

	bool InputsGeneralizer::focusLose(InputWindow in)
	{
		GENERALIZER(FocusLose);
	}

	bool InputsGeneralizer::windowClose(InputWindow in)
	{
		GENERALIZER(WindowClose);
	}

	bool InputsGeneralizer::windowShow(InputWindow in)
	{
		GENERALIZER(WindowShow);
	}

	bool InputsGeneralizer::windowHide(InputWindow in)
	{
		GENERALIZER(WindowHide);
	}

	bool InputsGeneralizer::windowMove(InputWindowValue in)
	{
		GENERALIZER(WindowMove);
	}

	bool InputsGeneralizer::windowResize(InputWindowValue in)
	{
		GENERALIZER(WindowResize);
	}

	bool InputsGeneralizer::mouseMove(InputMouse in)
	{
		GENERALIZER(MouseMove);
	}

	bool InputsGeneralizer::mousePress(InputMouse in)
	{
		GENERALIZER(MousePress);
	}

	bool InputsGeneralizer::mouseDoublePress(InputMouse in)
	{
		GENERALIZER(MouseDoublePress);
	}

	bool InputsGeneralizer::mouseRelease(InputMouse in)
	{
		GENERALIZER(MouseRelease);
	}

	bool InputsGeneralizer::mouseWheel(InputMouseWheel in)
	{
		GENERALIZER(MouseWheel);
	}

	bool InputsGeneralizer::keyPress(InputKey in)
	{
		GENERALIZER(KeyPress);
	}

	bool InputsGeneralizer::keyRelease(InputKey in)
	{
		GENERALIZER(KeyRelease);
	}

	bool InputsGeneralizer::keyRepeat(InputKey in)
	{
		GENERALIZER(KeyRepeat);
	}

	bool InputsGeneralizer::keyChar(InputKey in)
	{
		GENERALIZER(KeyChar);
	}

	bool InputsGeneralizer::guiWidget(InputGuiWidget in)
	{
		GENERALIZER(GuiWidget);
	}

	bool InputsGeneralizer::custom(const GenericInput::Any &in)
	{
		GENERALIZER(Custom);
	}

	bool InputsGeneralizer::unknown(const GenericInput &in)
	{
		return dispatcher.dispatch(in);
	}

#undef GENERALIZER

	bool InputsDispatchers::dispatch(const GenericInput &in)
	{
#define EVENT(CLASS, NAME, TYPE) \
		if (in.type == InputClassEnum::CLASS && in.data.type() == detail::typeIndex<TYPE>()) \
			return NAME.dispatch(in.data.get<TYPE>());

		EVENT(FocusGain, focusGain, InputWindow);
		EVENT(FocusLose, focusLose, InputWindow);
		EVENT(WindowClose, windowClose, InputWindow);
		EVENT(WindowShow, windowShow, InputWindow);
		EVENT(WindowHide, windowHide, InputWindow);
		EVENT(WindowMove, windowMove, InputWindowValue);
		EVENT(WindowResize, windowResize, InputWindowValue);
		EVENT(MouseMove, mouseMove, InputMouse);
		EVENT(MousePress, mousePress, InputMouse);
		EVENT(MouseDoublePress, mouseDoublePress, InputMouse);
		EVENT(MouseRelease, mouseRelease, InputMouse);
		EVENT(MouseWheel, mouseWheel, InputMouseWheel);
		EVENT(KeyPress, keyPress, InputKey);
		EVENT(KeyRelease, keyRelease, InputKey);
		EVENT(KeyRepeat, keyRepeat, InputKey);
		EVENT(KeyChar, keyChar, InputKey);
		EVENT(GuiWidget, guiWidget, InputGuiWidget);
#undef EVENT

		if (in.type == InputClassEnum::Custom)
			return custom.dispatch(in.data);

		return unknown.dispatch(in);
	}

	void InputsListeners::attach(InputsDispatchers *dispatchers, sint32 order)
	{
#define EVENT(CLASS, NAME, TYPE) \
		if (dispatchers) \
			NAME.attach(dispatchers->NAME, order); \
		else \
			NAME.detach();

		EVENT(FocusGain, focusGain, InputWindow);
		EVENT(FocusLose, focusLose, InputWindow);
		EVENT(WindowClose, windowClose, InputWindow);
		EVENT(WindowShow, windowShow, InputWindow);
		EVENT(WindowHide, windowHide, InputWindow);
		EVENT(WindowMove, windowMove, InputWindowValue);
		EVENT(WindowResize, windowResize, InputWindowValue);
		EVENT(MouseMove, mouseMove, InputMouse);
		EVENT(MousePress, mousePress, InputMouse);
		EVENT(MouseDoublePress, mouseDoublePress, InputMouse);
		EVENT(MouseRelease, mouseRelease, InputMouse);
		EVENT(MouseWheel, mouseWheel, InputMouseWheel);
		EVENT(KeyPress, keyPress, InputKey);
		EVENT(KeyRelease, keyRelease, InputKey);
		EVENT(KeyRepeat, keyRepeat, InputKey);
		EVENT(KeyChar, keyChar, InputKey);
		EVENT(GuiWidget, guiWidget, InputGuiWidget);

		EVENT(Custom, custom, const GenericInput::Any &);
		EVENT(Unknown, unknown, const GenericInput &);
#undef EVENT
	}

	void InputsListeners::bind(InputsGeneralizer *generalizer)
	{
#define EVENT(CLASS, NAME, TYPE) \
		if (generalizer) \
			NAME.bind<InputsGeneralizer, &InputsGeneralizer::NAME>(generalizer); \
		else \
			NAME.clear();

		EVENT(FocusGain, focusGain, InputWindow);
		EVENT(FocusLose, focusLose, InputWindow);
		EVENT(WindowClose, windowClose, InputWindow);
		EVENT(WindowShow, windowShow, InputWindow);
		EVENT(WindowHide, windowHide, InputWindow);
		EVENT(WindowMove, windowMove, InputWindowValue);
		EVENT(WindowResize, windowResize, InputWindowValue);
		EVENT(MouseMove, mouseMove, InputMouse);
		EVENT(MousePress, mousePress, InputMouse);
		EVENT(MouseDoublePress, mouseDoublePress, InputMouse);
		EVENT(MouseRelease, mouseRelease, InputMouse);
		EVENT(MouseWheel, mouseWheel, InputMouseWheel);
		EVENT(KeyPress, keyPress, InputKey);
		EVENT(KeyRelease, keyRelease, InputKey);
		EVENT(KeyRepeat, keyRepeat, InputKey);
		EVENT(KeyChar, keyChar, InputKey);
		EVENT(GuiWidget, guiWidget, InputGuiWidget);

		EVENT(Custom, custom, const GenericInput::Any &);
		EVENT(Unknown, unknown, const GenericInput &);
#undef EVENT
	}
}
