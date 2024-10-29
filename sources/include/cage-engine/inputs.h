#ifndef guard_inputs_juhgf98ser4g
#define guard_inputs_juhgf98ser4g

#include <cage-core/any.h>
#include <cage-core/events.h>
#include <cage-engine/core.h>

namespace cage
{
	class Window;
	class Gamepad;
	class VirtualReality;
	class VirtualRealityController;
	class GuiManager;
	class Entity;

	namespace input
	{
		// window

		namespace privat
		{
			struct BaseWindow
			{
				Window *window = nullptr;
			};
		}
		struct WindowFocusGain : privat::BaseWindow
		{};
		struct WindowFocusLose : privat::BaseWindow
		{};
		struct WindowClose : privat::BaseWindow
		{};
		struct WindowShow : privat::BaseWindow
		{};
		struct WindowHide : privat::BaseWindow
		{};
		struct WindowMove
		{
			Window *window = nullptr;
			Vec2i position;
		};
		struct WindowResize
		{
			Window *window = nullptr;
			Vec2i size;
		};

		// mouse

		namespace privat
		{
			struct BaseMouse
			{
				Window *window = nullptr;
				Vec2 position;
				MouseButtonsFlags buttons = MouseButtonsFlags::None;
				ModifiersFlags mods = ModifiersFlags::None;
				bool relative = false;
			};
		}
		struct MouseMove : privat::BaseMouse
		{};
		struct MouseRelativeMove : privat::BaseMouse
		{};
		struct MousePress : privat::BaseMouse
		{};
		struct MouseDoublePress : privat::BaseMouse
		{};
		struct MouseRelease : privat::BaseMouse
		{};
		struct MouseWheel
		{
			Window *window = nullptr;
			Vec2 position;
			Real wheel;
			ModifiersFlags mods = ModifiersFlags::None;
			bool relative = false;
		};

		// keyboard

		namespace privat
		{
			struct BaseKey
			{
				Window *window = nullptr;
				uint32 key = 0; // glfw key or utf-32 character
				ModifiersFlags mods = ModifiersFlags::None;
			};
		}
		struct KeyPress : privat::BaseKey
		{};
		struct KeyRelease : privat::BaseKey
		{};
		struct KeyRepeat : privat::BaseKey
		{};
		struct Character
		{
			Window *window = nullptr;
			uint32 character = 0; // utf-32 character
			ModifiersFlags mods = ModifiersFlags::None;
		};

		// gamepad

		namespace privat
		{
			struct BaseGamepad
			{
				Gamepad *gamepad = nullptr;
			};
			struct BaseGamepadKey
			{
				Gamepad *gamepad = nullptr;
				uint32 key = 0;
			};
		}
		struct GamepadConnected : privat::BaseGamepad
		{};
		struct GamepadDisconnected : privat::BaseGamepad
		{};
		struct GamepadPress : privat::BaseGamepadKey
		{};
		struct GamepadRelease : privat::BaseGamepadKey
		{};
		struct GamepadAxis
		{
			Gamepad *gamepad = nullptr;
			uint32 axis = 0;
			Real value;
		};

		// headset

		namespace privat
		{
			struct BaseHeadset
			{
				VirtualReality *headset = nullptr;
			};
		}
		struct HeadsetConnected : privat::BaseHeadset
		{};
		struct HeadsetDisconnected : privat::BaseHeadset
		{};
		struct HeadsetPose
		{
			Transform pose; // in local space of the virtual reality
			VirtualReality *headset = nullptr;
		};

		// controller

		namespace privat
		{
			struct BaseController
			{
				VirtualRealityController *controller = nullptr;
			};
			struct BaseControllerKey
			{
				VirtualRealityController *controller = nullptr;
				uint32 key = 0;
			};
		}
		struct ControllerConnected : privat::BaseController
		{};
		struct ControllerDisconnected : privat::BaseController
		{};
		struct ControllerPose
		{
			Transform pose; // grip pose in local space of the virtual reality
			VirtualRealityController *controller = nullptr;
		};
		struct ControllerPress : privat::BaseControllerKey
		{};
		struct ControllerRelease : privat::BaseControllerKey
		{};
		struct ControllerAxis
		{
			VirtualRealityController *controller = nullptr;
			uint32 axis = 0;
			Real value;
		};

		// gui

		struct GuiValue
		{
			GuiManager *manager = nullptr;
			Entity *entity = nullptr;
		};
		struct GuiInputConfirm
		{
			GuiManager *manager = nullptr;
			Entity *entity = nullptr;
		};
	}

	struct GenericInput : detail::AnyBase<44>
	{
		using detail::AnyBase<44>::AnyBase;
	};

	template<class T>
	concept InputFilterConcept = detail::AnyValueConcept<T, GenericInput::MaxSize>;

	template<InputFilterConcept T, class Callable>
	requires((std::is_invocable_r_v<bool, Callable, T> || std::is_invocable_r_v<void, Callable, T>) && !std::is_same_v<T, GenericInput>)
	auto inputFilter(Callable &&callable)
	{
		return [cl = std::move(callable)](const GenericInput &in) -> bool
		{
			if (!in.has<T>())
				return false;
			if constexpr (std::is_same_v<std::invoke_result_t<Callable, T>, bool>)
				return cl(in.get<T>());
			else
			{
				cl(in.get<T>());
				return false;
			}
		};
	}

	namespace privat
	{
		template<class>
		struct ExtractParam;
		template<class R, class T, class P>
		struct ExtractParam<R (T::*)(P)>
		{
			using Param = P;
		};
		template<class R, class T, class P>
		struct ExtractParam<R (T::*)(P) const>
		{
			using Param = P;
		};
		template<class R, class P>
		struct ExtractParam<R(P)>
		{
			using Param = P;
		};
	}

	template<class Callable>
	auto inputFilter(Callable &&callable)
	{
		using Param = typename privat::ExtractParam<decltype(&Callable::operator())>::Param;
		return inputFilter<Param, Callable>(std::move(callable));
	}

	template<class Func>
	auto inputFilter(Func *func)
	{
		using Param = typename privat::ExtractParam<Func>::Param;
		return inputFilter<Param>([func](const Param &in) { return func(in); });
	}
}

#endif // guard_inputs_juhgf98ser4g
