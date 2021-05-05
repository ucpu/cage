#include <cage-core/swapBufferGuard.h>
#include <cage-core/memoryAllocators.h>

#include "../private.h"

namespace cage
{
	namespace privat
	{
		GuiComponents::GuiComponents(EntityManager* ents)
		{
			detail::memset(this, 0, sizeof(*this));
#define GCHL_GENERATE(T) T = ents->defineComponent<CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(CAGE_JOIN(Gui, CAGE_JOIN(T, Component))());
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_COMMON_COMPONENTS, GCHL_GUI_WIDGET_COMPONENTS, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		}
	}

	GuiImpl::GraphicsData::GraphicsData() :
		debugShader(nullptr), elementShader(nullptr), fontShader(nullptr), imageAnimatedShader(nullptr), imageStaticShader(nullptr),
		colorPickerShader{nullptr, nullptr, nullptr},
		debugModel(nullptr), elementModel(nullptr), fontModel(nullptr), imageModel(nullptr)
	{}

	GuiImpl::EmitData::EmitData(const GuiCreateConfig &config) : arena(newMemoryAllocatorLinear({})), memory(+arena)
	{}

	GuiImpl::EmitData::~EmitData()
	{
		flush();
	}

	void GuiImpl::EmitData::flush()
	{
		first = last = nullptr;
		memory.flush();
	}

	GuiImpl::GuiImpl(const GuiCreateConfig &config) :
		entityMgr(newEntityManager()), components(entityMgr.get()),
		itemsArena(newMemoryAllocatorLinear({})), itemsMemory(+itemsArena), root(nullptr),
		emitData{config, config, config}, emitControl(nullptr),
		assetMgr(config.assetMgr),
		focusName(0), focusParts(0), hover(nullptr), eventsEnabled(false),
		zoom(1)
	{
		listeners.windowResize.bind<Gui, &Gui::windowResize>(this);
		listeners.mousePress.bind<Gui, &Gui::mousePress>(this);
		listeners.mouseDouble.bind<Gui, &Gui::mouseDouble>(this);
		listeners.mouseRelease.bind<Gui, &Gui::mouseRelease>(this);
		listeners.mouseMove.bind<Gui, &Gui::mouseMove>(this);
		listeners.mouseWheel.bind<Gui, &Gui::mouseWheel>(this);
		listeners.keyPress.bind<Gui, &Gui::keyPress>(this);
		listeners.keyRepeat.bind<Gui, &Gui::keyRepeat>(this);
		listeners.keyRelease.bind<Gui, &Gui::keyRelease>(this);
		listeners.keyChar.bind<Gui, &Gui::keyChar>(this);

		skins.resize(config.skinsCount);

		{
			SwapBufferGuardCreateConfig cfg(3);
			cfg.repeatedReads = true;
			emitController = newSwapBufferGuard(cfg);
		}
	}

	GuiImpl::~GuiImpl()
	{
		focusName = 0;
		hover = nullptr;
		itemsMemory.flush();
	}

	void GuiImpl::scaling()
	{
		pointsScale = zoom * retina;
		outputSize = vec2(outputResolution[0], outputResolution[1]) / pointsScale;
		outputMouse = vec2(inputMouse[0], inputMouse[1]) / pointsScale;
	}

	vec4 GuiImpl::pointsToNdc(vec2 position, vec2 size)
	{
		CAGE_ASSERT(size[0] >= 0 && size[1] >= 0);
		position *= pointsScale;
		size *= pointsScale;
		vec2 os = outputSize * pointsScale;
		vec2 invScreenSize = 2 / os;
		vec2 resPos = (position - os * 0.5) * invScreenSize;
		vec2 resSiz = size * invScreenSize;
		resPos[1] = -resPos[1] - resSiz[1];
		return vec4(resPos, resPos + resSiz);
	}

	uint32 GuiImpl::entityWidgetsCount(Entity *e)
	{
		GuiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_WIDGET_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}

	uint32 GuiImpl::entityLayoutsCount(Entity *e)
	{
		GuiImpl *impl = this;
		uint32 result = 0;
#define GCHL_GENERATE(T) if (GUI_HAS_COMPONENT(T, e)) result++;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, GCHL_GUI_LAYOUT_COMPONENTS));
#undef GCHL_GENERATE
		return result;
	}
}
