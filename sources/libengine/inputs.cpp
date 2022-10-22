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

	bool InputsGeneralizer::gamepadConnected(InputGamepadState in)
	{
		GENERALIZER(GamepadConnected);
	}

	bool InputsGeneralizer::gamepadDisconnected(InputGamepadState in)
	{
		GENERALIZER(GamepadDisconnected);
	}

	bool InputsGeneralizer::gamepadPress(InputGamepadKey in)
	{
		GENERALIZER(GamepadPress);
	}

	bool InputsGeneralizer::gamepadRelease(InputGamepadKey in)
	{
		GENERALIZER(GamepadRelease);
	}

	bool InputsGeneralizer::gamepadAxis(InputGamepadAxis in)
	{
		GENERALIZER(GamepadAxis);
	}

	bool InputsGeneralizer::headsetConnected(InputHeadsetState in)
	{
		GENERALIZER(HeadsetConnected);
	}

	bool InputsGeneralizer::headsetDisconnected(InputHeadsetState in)
	{
		GENERALIZER(HeadsetDisconnected);
	}

	bool InputsGeneralizer::controllerConnected(InputControllerState in)
	{
		GENERALIZER(ControllerConnected);
	}

	bool InputsGeneralizer::controllerDisconnected(InputControllerState in)
	{
		GENERALIZER(ControllerDisconnected);
	}

	bool InputsGeneralizer::controllerPress(InputControllerKey in)
	{
		GENERALIZER(ControllerPress);
	}

	bool InputsGeneralizer::controllerRelease(InputControllerKey in)
	{
		GENERALIZER(ControllerRelease);
	}

	bool InputsGeneralizer::controllerAxis(InputControllerAxis in)
	{
		GENERALIZER(ControllerAxis);
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
		if (in.type == InputClassEnum::CLASS && in.data.typeHash() == detail::typeHash<TYPE>()) \
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
		EVENT(GamepadConnected, gamepadConnected, InputGamepadState);
		EVENT(GamepadDisconnected, gamepadDisconnected, InputGamepadState);
		EVENT(GamepadPress, gamepadPress, InputGamepadKey);
		EVENT(GamepadRelease, gamepadRelease, InputGamepadKey);
		EVENT(GamepadAxis, gamepadAxis, InputGamepadAxis);
		EVENT(HeadsetConnected, headsetConnected, InputHeadsetState);
		EVENT(HeadsetDisconnected, headsetDisconnected, InputHeadsetState);
		EVENT(ControllerConnected, controllerConnected, InputControllerState);
		EVENT(ControllerDisconnected, controllerDisconnected, InputControllerState);
		EVENT(ControllerPress, controllerPress, InputControllerKey);
		EVENT(ControllerRelease, controllerRelease, InputControllerKey);
		EVENT(ControllerAxis, controllerAxis, InputControllerAxis);
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
		EVENT(GamepadConnected, gamepadConnected, InputGamepadState);
		EVENT(GamepadDisconnected, gamepadDisconnected, InputGamepadState);
		EVENT(GamepadPress, gamepadPress, InputGamepadKey);
		EVENT(GamepadRelease, gamepadRelease, InputGamepadKey);
		EVENT(GamepadAxis, gamepadAxis, InputGamepadAxis);
		EVENT(HeadsetConnected, headsetConnected, InputHeadsetState);
		EVENT(HeadsetDisconnected, headsetDisconnected, InputHeadsetState);
		EVENT(ControllerConnected, controllerConnected, InputControllerState);
		EVENT(ControllerDisconnected, controllerDisconnected, InputControllerState);
		EVENT(ControllerPress, controllerPress, InputControllerKey);
		EVENT(ControllerRelease, controllerRelease, InputControllerKey);
		EVENT(ControllerAxis, controllerAxis, InputControllerAxis);
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
		EVENT(GamepadConnected, gamepadConnected, InputGamepadState);
		EVENT(GamepadDisconnected, gamepadDisconnected, InputGamepadState);
		EVENT(GamepadPress, gamepadPress, InputGamepadKey);
		EVENT(GamepadRelease, gamepadRelease, InputGamepadKey);
		EVENT(GamepadAxis, gamepadAxis, InputGamepadAxis);
		EVENT(HeadsetConnected, headsetConnected, InputHeadsetState);
		EVENT(HeadsetDisconnected, headsetDisconnected, InputHeadsetState);
		EVENT(ControllerConnected, controllerConnected, InputControllerState);
		EVENT(ControllerDisconnected, controllerDisconnected, InputControllerState);
		EVENT(ControllerPress, controllerPress, InputControllerKey);
		EVENT(ControllerRelease, controllerRelease, InputControllerKey);
		EVENT(ControllerAxis, controllerAxis, InputControllerAxis);
		EVENT(GuiWidget, guiWidget, InputGuiWidget);

		EVENT(Custom, custom, const GenericInput::Any &);
		EVENT(Unknown, unknown, const GenericInput &);
#undef EVENT
	}
}
