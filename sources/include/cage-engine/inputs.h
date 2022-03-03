#ifndef guard_inputs_juhgf98ser4g
#define guard_inputs_juhgf98ser4g

#include <cage-core/any.h>
#include <cage-core/events.h>

#include "core.h"

namespace cage
{
	enum class InputClassEnum : uint32
	{
		None = 0,
		FocusGain, FocusLose, WindowClose, WindowShow, WindowHide, // InputWindow
		WindowMove, WindowResize, // InputWindowValue
		MouseMove, MousePress, MouseDoublePress, MouseRelease, // InputMouse
		MouseWheel, // InputMouseWheel
		KeyPress, KeyRelease, KeyRepeat, KeyChar, // InputKey
		GamepadConnected, GamepadDisconnected, // InputGamepadState
		GamepadPress, GamepadRelease, // InputGamepadKey
		GamepadAxis, // InputGamepadAxis
		GuiWidget, // InputGuiWidget
		Custom = 1000,
	};

	struct InputWindow
	{
		Window *window = nullptr;
	};

	struct InputWindowValue
	{
		Window *window = nullptr;
		Vec2i value;
	};

	struct InputMouse
	{
		Window *window = nullptr;
		Vec2i position;
		MouseButtonsFlags buttons = MouseButtonsFlags::None;
		ModifiersFlags mods = ModifiersFlags::None;
	};

	struct InputMouseWheel
	{
		Window *window = nullptr;
		Vec2i position;
		sint32 wheel = 0;
		ModifiersFlags mods = ModifiersFlags::None;
	};

	struct InputKey
	{
		Window *window = nullptr;
		uint32 key = 0; // glfw key or utf-32 character
		ModifiersFlags mods = ModifiersFlags::None;
	};

	struct InputGamepadState
	{
		Gamepad *gamepad = nullptr;
	};

	struct InputGamepadKey
	{
		Gamepad *gamepad = nullptr;
		uint32 key = 0;
	};

	struct InputGamepadAxis
	{
		Gamepad *gamepad = nullptr;
		uint32 axis = 0;
		Real value;
	};

	struct InputGuiWidget
	{
		GuiManager *manager = nullptr;
		uint32 widget = 0;
	};

	struct GenericInput
	{
		using Any = detail::AnyBase<32>;
		Any data;
		InputClassEnum type = InputClassEnum::None;
	};

	struct CAGE_ENGINE_API InputsGeneralizer
	{
		bool focusGain(InputWindow);
		bool focusLose(InputWindow);
		bool windowClose(InputWindow);
		bool windowShow(InputWindow);
		bool windowHide(InputWindow);
		bool windowMove(InputWindowValue);
		bool windowResize(InputWindowValue);
		bool mouseMove(InputMouse);
		bool mousePress(InputMouse);
		bool mouseDoublePress(InputMouse);
		bool mouseRelease(InputMouse);
		bool mouseWheel(InputMouseWheel);
		bool keyPress(InputKey);
		bool keyRelease(InputKey);
		bool keyRepeat(InputKey);
		bool keyChar(InputKey);
		bool gamepadConnected(InputGamepadState);
		bool gamepadDisconnected(InputGamepadState);
		bool gamepadPress(InputGamepadKey);
		bool gamepadRelease(InputGamepadKey);
		bool gamepadAxis(InputGamepadAxis);
		bool guiWidget(InputGuiWidget);
		bool custom(const GenericInput::Any &);
		bool unknown(const GenericInput &);

		EventDispatcher<bool(const GenericInput &)> dispatcher;
	};

	struct CAGE_ENGINE_API InputsDispatchers
	{
		EventDispatcher<bool(InputWindow)> focusGain, focusLose, windowClose, windowShow, windowHide;
		EventDispatcher<bool(InputWindowValue)> windowMove, windowResize;
		EventDispatcher<bool(InputMouse)> mouseMove, mousePress, mouseDoublePress, mouseRelease;
		EventDispatcher<bool(InputMouseWheel)> mouseWheel;
		EventDispatcher<bool(InputKey)> keyPress, keyRelease, keyRepeat, keyChar;
		EventDispatcher<bool(InputGamepadState)> gamepadConnected, gamepadDisconnected;
		EventDispatcher<bool(InputGamepadKey)> gamepadPress, gamepadRelease;
		EventDispatcher<bool(InputGamepadAxis)> gamepadAxis;
		EventDispatcher<bool(InputGuiWidget)> guiWidget;
		EventDispatcher<bool(const GenericInput::Any &)> custom;
		EventDispatcher<bool(const GenericInput &)> unknown;

		bool dispatch(const GenericInput &input);
	};

	struct CAGE_ENGINE_API InputsListeners
	{
		EventListener<bool(InputWindow)> focusGain, focusLose, windowClose, windowShow, windowHide;
		EventListener<bool(InputWindowValue)> windowMove, windowResize;
		EventListener<bool(InputMouse)> mouseMove, mousePress, mouseDoublePress, mouseRelease;
		EventListener<bool(InputMouseWheel)> mouseWheel;
		EventListener<bool(InputKey)> keyPress, keyRelease, keyRepeat, keyChar;
		EventListener<bool(InputGamepadState)> gamepadConnected, gamepadDisconnected;
		EventListener<bool(InputGamepadKey)> gamepadPress, gamepadRelease;
		EventListener<bool(InputGamepadAxis)> gamepadAxis;
		EventListener<bool(InputGuiWidget)> guiWidget;
		EventListener<bool(const GenericInput::Any &)> custom;
		EventListener<bool(const GenericInput &)> unknown;

		void attach(InputsDispatchers *dispatchers, sint32 order = 0);
		void bind(InputsGeneralizer *generalizer);
	};

	template<InputClassEnum C, class T, class R = void>
	struct InputListener : private EventListener<R(const GenericInput &)>, private Delegate<R(T)>
	{
		static_assert(std::is_same_v<R, void> || std::is_same_v<R, bool>);

		explicit InputListener(const std::source_location &location = std::source_location::current()) : EventListener<R(const GenericInput &)>(location)
		{
			EventListener<R(const GenericInput &)>::template bind<InputListener, &InputListener::run>(this);
		}

		using EventListener<R(const GenericInput &)>::attach;
		using EventListener<R(const GenericInput &)>::detach;
		using Delegate<R(T)>::bind;
		using Delegate<R(T)>::clear;

	private:
		template<class G = R, typename = std::enable_if_t<std::is_same_v<G, bool>>>
		bool run(const GenericInput &in)
		{
			if (in.type == C && in.data.typeHash() == detail::typeHash<T>())
				return (*(Delegate<R(T)> *)this)(in.data.get<T>());
			return false;
		}

		template<class G = R, typename = std::enable_if_t<std::is_same_v<G, void>>>
		void run(const GenericInput &in)
		{
			if (in.type == C && in.data.typeHash() == detail::typeHash<T>())
				(*(Delegate<R(T)> *)this)(in.data.get<T>());
		}
	};
}

#endif // guard_inputs_juhgf98ser4g
