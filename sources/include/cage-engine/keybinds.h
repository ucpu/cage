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
		Press = 1 << 0,
		Repeat = 1 << 1,
		Double = 1 << 2,
		Release = 1 << 3,
		Scroll = 1 << 4,
		Tick = 1 << 5,
	};
	GCHL_ENUM_BITS(KeybindModesFlags);

	struct CAGE_ENGINE_API KeybindCreateConfig
	{
		String id;
		sint32 eventOrder = 0;
		sint32 displayOrder = 0;
		ModifiersFlags requiredFlags = ModifiersFlags::None;
		ModifiersFlags forbiddenFlags = ModifiersFlags::Super;
		KeybindDevicesFlags devices = KeybindDevicesFlags::Keyboard | KeybindDevicesFlags::Mouse | KeybindDevicesFlags::Wheel;
		KeybindModesFlags modes = KeybindModesFlags::Press;
	};

	CAGE_ENGINE_API Holder<Keybind> newKeybind(const KeybindCreateConfig &config, const GenericInput &defaults = {}, Delegate<bool(const GenericInput &)> event = {});
	CAGE_ENGINE_API Holder<Keybind> newKeybind(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults, Delegate<bool(const GenericInput &)> event = {});
	CAGE_ENGINE_API Keybind *findKeybind(const String &id);

	CAGE_ENGINE_API void keybindsRegisterListeners(EventDispatcher<bool(const GenericInput &)> &dispatcher);

	namespace input
	{
		struct CAGE_ENGINE_API Tick
		{};
	}
	CAGE_ENGINE_API void keybindsDispatchTick();

	CAGE_ENGINE_API void keybindsGuiWidget(guiBuilder::GuiBuilder *g, Keybind *keybind);
	CAGE_ENGINE_API void keybindsGuiTable(guiBuilder::GuiBuilder *g, const String &filterPrefix = {});

	CAGE_ENGINE_API Holder<Ini> keybindsExport();
	CAGE_ENGINE_API void keybindsImport(const Ini *ini);
}

#endif
