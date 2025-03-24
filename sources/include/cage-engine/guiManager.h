#ifndef guard_guiManager_xf1g9ret213sdr45zh
#define guard_guiManager_xf1g9ret213sdr45zh

#include <cage-engine/inputs.h>

namespace cage
{
	class EntityManager;
	class RenderQueue;
	class ProvisionalGraphics;
	class SoundsQueue;
	struct GuiSkinConfig;
	struct GuiSkinIndex;

	class CAGE_ENGINE_API GuiManager : private Immovable
	{
	public:
		void outputResolution(Vec2i resolution); // pixels
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

		bool handleInput(const GenericInput &);
		void invalidateInputs(); // skip all remaining inputs until next prepare
		EventDispatcher<bool(const GenericInput &)> widgetEvent; // called from inside handleInput

		GuiSkinConfig &skin(GuiSkinIndex index);
		const GuiSkinConfig &skin(GuiSkinIndex index) const;

		EntityManager *entities();
	};

	struct CAGE_ENGINE_API GuiManagerCreateConfig
	{
		AssetsManager *assetManager = nullptr;
		ProvisionalGraphics *provisionalGraphics = nullptr;
		SoundsQueue *soundsQueue = nullptr;
		uint32 skinsCount = 4;
		bool tooltipsEnabled = true;
	};

	CAGE_ENGINE_API Holder<GuiManager> newGuiManager(const GuiManagerCreateConfig &config);
}

#endif // guard_guiManager_xf1g9ret213sdr45zh
