#ifndef guard_guiManager_xf1g9ret213sdr45zh
#define guard_guiManager_xf1g9ret213sdr45zh

#include <cage-core/events.h>

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API GuiManager : private Immovable
	{
	public:
		void outputResolution(const Vec2i &resolution); // pixels
		Vec2i outputResolution() const;
		void outputRetina(Real retina); // pixels per point (1D)
		Real outputRetina() const;
		void zoom(Real zoom); // pixels per point (1D)
		Real zoom() const;
		void focus(uint32 widget);
		uint32 focus() const;

		void prepare(); // prepare the gui for handling events
		Holder<RenderQueue> finish(); // finish handling events, generate rendering commands, and release resources
		void cleanUp();

		bool windowResize(const Vec2i &);
		bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, const Vec2i &point);
		bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, const Vec2i &point);
		bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, const Vec2i &point);
		bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, const Vec2i &point);
		bool mouseWheel(sint32 wheel, ModifiersFlags modifiers, const Vec2i &point);
		bool keyPress(uint32 key, ModifiersFlags modifiers);
		bool keyRepeat(uint32 key, ModifiersFlags modifiers);
		bool keyRelease(uint32 key, ModifiersFlags modifiers);
		bool keyChar(uint32 key);

		void handleWindowEvents(Window *window, sint32 order = 0);
		void skipAllEventsUntilNextUpdate();
		Vec2i inputResolution() const;
		Delegate<bool(const Vec2i&, Vec2&)> eventCoordinatesTransformer; // called from controlUpdateStart or from any window events, it should return false to signal that the point is outside the gui, otherwise the point should be converted from window coordinate system to the gui output resolution coordinate system
		EventDispatcher<bool(uint32)> widgetEvent; // called from controlUpdateStart or window events

		GuiSkinConfig &skin(uint32 index = 0);
		const GuiSkinConfig &skin(uint32 index = 0) const;

		EntityManager *entities();
	};

	struct CAGE_ENGINE_API GuiManagerCreateConfig
	{
		AssetManager *assetMgr = nullptr;
		uint32 skinsCount = 1;
	};

	CAGE_ENGINE_API Holder<GuiManager> newGuiManager(const GuiManagerCreateConfig &config);
}

#endif // guard_guiManager_xf1g9ret213sdr45zh
