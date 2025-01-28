#ifndef guard_keybinds_h_df5h456sdcvr5u8io
#define guard_keybinds_h_df5h456sdcvr5u8io

#include <cage-engine/inputs.h>

namespace cage
{
	class Ini;
	namespace guiBuilder
	{
		class GuiBuilder;
	}

	class CAGE_ENGINE_API Keybind : private Immovable
	{
	public:
		const String &id() const;
		String name() const;
		String value(uint32 index = 0) const;
		bool process(const GenericInput &input) const;
		void setActive(bool a);
		bool active() const;

		uint32 count() const;
		void override(uint32 index, const GenericInput &input);
		void add(const GenericInput &input = {});
		void remove(uint32 index);
		void clear();
		void reset(); // uses defaults

		Delegate<bool(const GenericInput &)> event;
	};

	enum class KeybindDevicesFlags : uint32
	{
		None = 0,
		Keyboard = 1 << 0,
		Mouse = 1 << 1,
		Wheel = 1 << 2,
		// todo controller
	};
	GCHL_ENUM_BITS(KeybindDevicesFlags);

	enum class KeybindModesFlags : uint32
	{
		None = 0,
		KeyPress = 1 << 0,
		KeyRepeat = 1 << 1,
		KeyRelease = 1 << 2,
		MousePress = 1 << 3,
		MouseDouble = 1 << 4,
		MouseRelease = 1 << 5,
		WheelScroll = 1 << 6,
		GameTick = 1 << 7,
		EngineTick = 1 << 8,
	};
	GCHL_ENUM_BITS(KeybindModesFlags);
	CAGE_ENGINE_API KeybindModesFlags keybindMode(const GenericInput &in);

	struct CAGE_ENGINE_API KeybindCreateConfig
	{
		String id;
		sint32 eventOrder = 0;
		sint32 displayOrder = 0;
		ModifiersFlags requiredFlags = ModifiersFlags::None;
		ModifiersFlags forbiddenFlags = ModifiersFlags::Super;
		bool exactFlags = true;
		KeybindDevicesFlags devices = KeybindDevicesFlags::Keyboard | KeybindDevicesFlags::Mouse; // which devices are allowed to bind
		KeybindModesFlags modes = KeybindModesFlags::KeyPress | KeybindModesFlags::MousePress; // which modes trigger the event (contrary: activation is always with press and release)
	};

	CAGE_ENGINE_API Holder<Keybind> newKeybind(const KeybindCreateConfig &config, const GenericInput &defaults = {}, Delegate<bool(const GenericInput &)> event = {});
	CAGE_ENGINE_API Holder<Keybind> newKeybind(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults, Delegate<bool(const GenericInput &)> event = {});
	CAGE_ENGINE_API Keybind *findKeybind(const String &id);
	CAGE_ENGINE_API void keybindsRegisterListeners(EventDispatcher<bool(const GenericInput &)> &dispatcher);

	namespace input
	{
		struct CAGE_ENGINE_API GameTick
		{};
		struct CAGE_ENGINE_API EngineTick
		{};
	}
	CAGE_ENGINE_API void keybindsDispatchGameTick();
	CAGE_ENGINE_API void keybindsDispatchEngineTick();

	CAGE_ENGINE_API void keybindsGuiWidget(guiBuilder::GuiBuilder *g, Keybind *keybind);
	CAGE_ENGINE_API void keybindsGuiTable(guiBuilder::GuiBuilder *g, const String &filterPrefix = {});

	CAGE_ENGINE_API Holder<Ini> keybindsExport();
	CAGE_ENGINE_API void keybindsImport(const Ini *ini);
}

#endif
