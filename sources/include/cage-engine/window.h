#ifndef guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC
#define guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC

#include <cage-engine/inputs.h>

namespace cage
{
	enum class WindowFlags : uint32
	{
		None = 0,
		Resizeable = 1 << 0,
		Border = 1 << 1,
	};

	class CAGE_ENGINE_API Window : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		String title() const;
		void title(const String &value);

		bool isFocused() const;
		bool isFullscreen() const;
		bool isMaximized() const;
		bool isWindowed() const; // visible, not maximized, and not fullscreen
		bool isMinimized() const;
		bool isHidden() const;
		bool isVisible() const; // not hidden and not minimized

		void setFullscreen(Vec2i resolution, uint32 frequency = 0, const String &screenId = "");
		void setMaximized();
		void setWindowed(WindowFlags flags = WindowFlags::Border | WindowFlags::Resizeable);
		void setMinimized();
		void setHidden();

		bool mouseVisible() const;
		void mouseVisible(bool visible);
		bool mouseRelativeMovement() const;
		void mouseRelativeMovement(bool relative);
		Vec2 mousePosition() const;
		void mousePosition(Vec2 pos);
		MouseButtonsFlags mouseButtons() const;

		ModifiersFlags keyboardModifiers() const;
		bool keyboardKey(uint32 key) const;

		Vec2i resolution() const;
		Real contentScaling() const;
		String screenId() const;

		Vec2i windowedSize() const;
		void windowedSize(Vec2i size);
		Vec2i windowedPosition() const;
		void windowedPosition(Vec2i pos);
	};

	struct CAGE_ENGINE_API WindowCreateConfig
	{};

	CAGE_ENGINE_API Holder<Window> newWindow(const WindowCreateConfig &config);

	CAGE_ENGINE_API detail::StringBase<27> getKeyName(uint32 key);
	CAGE_ENGINE_API detail::StringBase<27> getButtonsNames(MouseButtonsFlags buttons);
	CAGE_ENGINE_API detail::StringBase<27> getModifiersNames(ModifiersFlags mods);

	CAGE_ENGINE_API uint32 findKeyWithCharacter(uint32 utf32character);
}

#endif
