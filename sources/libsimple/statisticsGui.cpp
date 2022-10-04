#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/debug.h>
#include <cage-engine/guiComponents.h>
#include <cage-engine/window.h>

#include <cage-simple/engine.h>
#include <cage-simple/statisticsGui.h>

namespace cage
{
	namespace
	{
		ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer");
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels");
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones");
		ConfigBool confProfilingEnabled("cage/profiling/enabled");
		ConfigFloat confRenderGamma("cage/graphics/gamma");

		class StatisticsGuiImpl : public StatisticsGui
		{
		public:
			InputListener<InputClassEnum::KeyPress, InputKey, bool> keyPressListener;
			InputListener<InputClassEnum::KeyRepeat, InputKey, bool> keyRepeatListener;
			EventListener<bool()> updateListener;

			uint32 panelIndex = 0;
			uint32 layoutIndex = 0;
			uint32 labelIndices[40] = {};
			const StatisticsGuiFlags *labelFlags = nullptr;
			const char *const *labelNames = nullptr;
			uint32 labelsCount = 0;
			StatisticsGuiScopeEnum profilingModeOld = StatisticsGuiScopeEnum::Full;

			StatisticsGuiImpl()
			{
				keyPressListener.bind<StatisticsGuiImpl, &StatisticsGuiImpl::keyPress>(this);
				keyPressListener.attach(engineWindow()->events);
				keyRepeatListener.bind<StatisticsGuiImpl, &StatisticsGuiImpl::keyRepeat>(this);
				keyRepeatListener.attach(engineWindow()->events);
				updateListener.bind<StatisticsGuiImpl, &StatisticsGuiImpl::update>(this);
				updateListener.attach(controlThread().update);
			}

			void nullData()
			{
				panelIndex = 0;
				layoutIndex = 0;
				for (uint32 i = 0; i < sizeof(labelIndices) / sizeof(labelIndices[0]); i++)
					labelIndices[i] = 0;
				labelFlags = nullptr;
				labelNames = nullptr;
				labelsCount = 0;
			}

			void generateEntities()
			{
				static constexpr StatisticsGuiFlags flagsFull[] = {
					StatisticsGuiFlags::Control,
					StatisticsGuiFlags::Sound,
					StatisticsGuiFlags::GraphicsPrepare,
					StatisticsGuiFlags::GraphicsDispatch,
					StatisticsGuiFlags::FrameTime,
					StatisticsGuiFlags::DrawCalls,
					StatisticsGuiFlags::DrawPrimitives,
					StatisticsGuiFlags::Entities,
				};
				static constexpr const char* namesFull[sizeof(flagsFull) / sizeof(flagsFull[0])] = {
					"Control: ",
					"Sound: ",
					"Graphics Prepare: ",
					"Graphics Dispatch: ",
					"Frame Time: ",
					"Draw Calls: ",
					"Draw Primitives: ",
					"Entities: ",
				};

				static constexpr StatisticsGuiFlags flagsShort[] = {
					StatisticsGuiFlags::Control,
					StatisticsGuiFlags::Sound,
					StatisticsGuiFlags::GraphicsPrepare,
					StatisticsGuiFlags::FrameTime,
				};
				static constexpr const char* namesShort[sizeof(flagsShort) / sizeof(flagsShort[0])] = {
					"Control: ",
					"Sound: ",
					"Prepare: ",
					"Frame: ",
				};

				static constexpr StatisticsGuiFlags flagsFps[] = {
					StatisticsGuiFlags::FrameTime,
				};
				static constexpr const char* namesFps[sizeof(flagsFps) / sizeof(flagsFps[0])] = {
					"Frame Time: ",
				};

				clearEntities();
				profilingModeOld = statisticsScope;
				EntityManager *g = engineGuiEntities();
				Entity *panel = g->createUnique();
				{
					panelIndex = panel->name();
					GuiScrollbarsComponent &sc = panel->value<GuiScrollbarsComponent>();
					sc.alignment = screenPosition;
				}
				Entity *layout = g->createUnique();
				{
					layoutIndex = layout->name();
					layout->value<GuiPanelComponent>();
					layout->value<GuiLayoutTableComponent>();
					GuiParentComponent &child = layout->value<GuiParentComponent>();
					child.parent = panel->name();
				}

				static constexpr uint32 labelsPerMode[] = {
					sizeof(flagsFull) / sizeof(flagsFull[0]),
					sizeof(flagsShort) / sizeof(flagsShort[0]),
					sizeof(flagsFps) / sizeof(flagsFps[0]),
					0 };
				labelsCount = labelsPerMode[(uint32)statisticsScope];
				for (uint32 i = 0; i < labelsCount * 2; i++)
				{
					Entity *e = g->createUnique();
					labelIndices[i] = e->name();
					e->value<GuiLabelComponent>();
					GuiParentComponent &child = e->value<GuiParentComponent>();
					child.parent = layout->name();
					child.order = i;
					if (i % 2 == 1)
					{
						GuiTextFormatComponent &tf = e->value<GuiTextFormatComponent>();
						tf.align = TextAlignEnum::Right;
					}
				}

				switch (statisticsScope)
				{
				case StatisticsGuiScopeEnum::None:
					labelFlags = nullptr;
					labelNames = nullptr;
					break;
				case StatisticsGuiScopeEnum::Fps:
					labelFlags = flagsFps;
					labelNames = namesFps;
					break;
				case StatisticsGuiScopeEnum::Short:
					labelFlags = flagsShort;
					labelNames = namesShort;
					break;
				case StatisticsGuiScopeEnum::Full:
					labelFlags = flagsFull;
					labelNames = namesFull;
					break;
				}
			}

			void destroyEntity(uint32 name)
			{
				if (name == 0)
					return;
				EntityManager *g = engineGuiEntities();
				if (!g->has(name))
					return;
				g->get(name)->destroy();
			}

			void clearEntities()
			{
				destroyEntity(panelIndex);
				destroyEntity(layoutIndex);
				for (uint32 i = 0; i < sizeof(labelIndices) / sizeof(labelIndices[0]); i++)
					destroyEntity(labelIndices[i]);
				nullData();
			}

			void checkEntities()
			{
				EntityManager *g = engineGuiEntities();
				const bool panelPresent = panelIndex != 0 && g->has(panelIndex);
				const bool visible = statisticsScope != StatisticsGuiScopeEnum::None;
				if (panelPresent != visible || profilingModeOld != statisticsScope)
				{ // change needed
					clearEntities();
					if (visible)
						generateEntities();
				}
			}

			void setTextLabel(uint32 index, const String &value)
			{
				CAGE_ASSERT(index < sizeof(labelIndices) / sizeof(labelIndices[0]));
				if (labelIndices[index] == 0 || !engineGuiEntities()->has(labelIndices[index]))
					return;
				Entity *timing = engineGuiEntities()->get(labelIndices[index]);
				GuiTextComponent &t = timing->value<GuiTextComponent>();
				t.value = value;
			}

			bool update()
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
				{
					setTextLabel(i * 2 + 0, labelNames[i]);
					if (labelFlags[i] <= StatisticsGuiFlags::FrameTime)
						setTextLabel(i * 2 + 1, Stringizer() + (engineStatisticsValues(labelFlags[i], statisticsMode) / 1000) + " ms");
					else
						setTextLabel(i * 2 + 1, Stringizer() + (engineStatisticsValues(labelFlags[i], statisticsMode)));
				}
				return false;
			}

			bool keyPress(InputKey in)
			{
				if (in.mods == keyModifiers)
				{
					if (in.key == keyToggleStatisticsScope)
					{
						switch (statisticsScope)
						{
						case StatisticsGuiScopeEnum::Full: statisticsScope = StatisticsGuiScopeEnum::Short; break;
						case StatisticsGuiScopeEnum::Short: statisticsScope = StatisticsGuiScopeEnum::Fps; break;
						case StatisticsGuiScopeEnum::Fps: statisticsScope = StatisticsGuiScopeEnum::None; break;
						case StatisticsGuiScopeEnum::None: statisticsScope = StatisticsGuiScopeEnum::Full; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling scope enum");
						}
						return true;
					}
					if (in.key == keyToggleStatisticsMode)
					{
						switch (statisticsMode)
						{
						case StatisticsGuiModeEnum::Average: statisticsMode = StatisticsGuiModeEnum::Maximum; break;
						case StatisticsGuiModeEnum::Maximum: statisticsMode = StatisticsGuiModeEnum::Last; break;
						case StatisticsGuiModeEnum::Last: statisticsMode = StatisticsGuiModeEnum::Average; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling mode enum");
						}
						return true;
					}
					if (in.key == keyToggleProfilingEnabled)
					{
						confProfilingEnabled = !confProfilingEnabled;
						return true;
					}
					if (in.key == keyVisualizeBufferPrev)
					{
						confVisualizeBuffer = confVisualizeBuffer - 1;
						return true;
					}
					if (in.key == keyVisualizeBufferNext)
					{
						confVisualizeBuffer = confVisualizeBuffer + 1;
						return true;
					}
					if (in.key == keyToggleRenderMissingModels)
					{
						confRenderMissingModels = !confRenderMissingModels;
						return true;
					}
					if (in.key == keyToggleRenderSkeletonBones)
					{
						confRenderSkeletonBones = !confRenderSkeletonBones;
						return true;
					}
				}
				return false;
			}

			bool keyRepeat(InputKey in)
			{
				if (in.mods == keyModifiers)
				{
					if (in.key == keyDecreaseGamma)
					{
						Real g = Real(confRenderGamma);
						g = max(1, g - 0.05);
						confRenderGamma = g.value;
						return true;
					}
					if (in.key == keyIncreaseGamma)
					{
						Real g = Real(confRenderGamma);
						g = min(5, g + 0.05);
						confRenderGamma = g.value;
						return true;
					}
				}
				return false;
			}
		};
	}

	Holder<StatisticsGui> newStatisticsGui()
	{
		return systemMemory().createImpl<StatisticsGui, StatisticsGuiImpl>();
	}
}
