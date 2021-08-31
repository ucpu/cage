#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/debug.h>

#include <cage-engine/engineStatistics.h>
#include <cage-engine/gui.h>
#include <cage-engine/engine.h>
#include <cage-engine/window.h>

namespace cage
{
	namespace
	{
		ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer");
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels");
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones");
		ConfigBool confProfilingEnabled("cage/profiling/enabled");

		class EngineStatisticsImpl : public EngineStatistics
		{
		public:
			EventListener<bool(uint32, ModifiersFlags)> keyPressListener;
			EventListener<bool()> updateListener;

			uint32 panelIndex;
			uint32 layoutIndex;
			uint32 labelIndices[40];
			const EngineStatisticsFlags *labelFlags;
			const char *const *labelNames;
			uint32 labelsCount;
			EngineStatisticsScopeEnum profilingModeOld;

			EngineStatisticsImpl() : profilingModeOld(EngineStatisticsScopeEnum::Full)
			{
				nullData();

				keyPressListener.bind<EngineStatisticsImpl, &EngineStatisticsImpl::keyPress>(this);
				updateListener.bind<EngineStatisticsImpl, &EngineStatisticsImpl::update>(this);

				engineWindow()->events.keyPress.attach(keyPressListener);
				controlThread().update.attach(updateListener);
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
				static constexpr EngineStatisticsFlags flagsFull[] = {
					EngineStatisticsFlags::Control,
					EngineStatisticsFlags::Sound,
					EngineStatisticsFlags::GraphicsPrepare,
					EngineStatisticsFlags::GraphicsDispatch,
					EngineStatisticsFlags::FrameTime,
					EngineStatisticsFlags::DrawCalls,
					EngineStatisticsFlags::DrawPrimitives,
					EngineStatisticsFlags::Entities,
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

				static constexpr EngineStatisticsFlags flagsShort[] = {
					EngineStatisticsFlags::Control,
					EngineStatisticsFlags::Sound,
					EngineStatisticsFlags::GraphicsPrepare,
					EngineStatisticsFlags::FrameTime,
				};
				static constexpr const char* namesShort[sizeof(flagsShort) / sizeof(flagsShort[0])] = {
					"Control: ",
					"Sound: ",
					"Prepare: ",
					"Frame: ",
				};

				static constexpr EngineStatisticsFlags flagsFps[] = {
					EngineStatisticsFlags::FrameTime,
				};
				static constexpr const char* namesFps[sizeof(flagsFps) / sizeof(flagsFps[0])] = {
					"Frame Time: ",
				};

				clearEntities();
				profilingModeOld = statisticsScope;
				EntityManager *g = engineGui()->entities();
				Entity *panel = g->createUnique();
				{
					panelIndex = panel->name();
					GuiScrollbarsComponent &sc = panel->value<GuiScrollbarsComponent>();
					sc.alignment = screenPosition;
				}
				Entity *layout = g->createUnique();
				{
					layoutIndex = layout->name();
					GuiPanelComponent &c = layout->value<GuiPanelComponent>();
					GuiLayoutTableComponent &l = layout->value<GuiLayoutTableComponent>();
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
					GuiLabelComponent &c = e->value<GuiLabelComponent>();
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
				case EngineStatisticsScopeEnum::None:
					labelFlags = nullptr;
					labelNames = nullptr;
					break;
				case EngineStatisticsScopeEnum::Fps:
					labelFlags = flagsFps;
					labelNames = namesFps;
					break;
				case EngineStatisticsScopeEnum::Short:
					labelFlags = flagsShort;
					labelNames = namesShort;
					break;
				case EngineStatisticsScopeEnum::Full:
					labelFlags = flagsFull;
					labelNames = namesFull;
					break;
				}
			}

			void destroyEntity(uint32 name)
			{
				if (name == 0)
					return;
				EntityManager *g = engineGui()->entities();
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
				EntityManager *g = engineGui()->entities();
				bool panelPresent = panelIndex != 0 && g->has(panelIndex);
				bool visible = statisticsScope != EngineStatisticsScopeEnum::None;
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
				if (labelIndices[index] == 0 || !engineGui()->entities()->has(labelIndices[index]))
					return;
				Entity *timing = engineGui()->entities()->get(labelIndices[index]);
				GuiTextComponent &t = timing->value<GuiTextComponent>();
				t.value = value;
			}

			bool update()
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
				{
					setTextLabel(i * 2 + 0, labelNames[i]);
					if (labelFlags[i] <= EngineStatisticsFlags::FrameTime)
						setTextLabel(i * 2 + 1, Stringizer() + (engineStatisticsValues(labelFlags[i], statisticsMode) / 1000) + " ms");
					else
						setTextLabel(i * 2 + 1, Stringizer() + (engineStatisticsValues(labelFlags[i], statisticsMode)));
				}
				return false;
			}

			void setWindowFullscreen(bool fullscreen)
			{
				if (fullscreen)
				{
					try
					{
						detail::OverrideBreakpoint ob;
						engineWindow()->setFullscreen(Vec2i(0, 0));
					}
					catch (...)
					{
						setWindowFullscreen(false);
					}
				}
				else
				{
					engineWindow()->setWindowed(WindowFlags::Resizeable | WindowFlags::Border);
				}
			}

			bool keyPress(uint32 key, ModifiersFlags mods)
			{
				if (mods == keyModifiers)
				{
					if (key == keyToggleStatisticsScope)
					{
						switch (statisticsScope)
						{
						case EngineStatisticsScopeEnum::Full: statisticsScope = EngineStatisticsScopeEnum::Short; break;
						case EngineStatisticsScopeEnum::Short: statisticsScope = EngineStatisticsScopeEnum::Fps; break;
						case EngineStatisticsScopeEnum::Fps: statisticsScope = EngineStatisticsScopeEnum::None; break;
						case EngineStatisticsScopeEnum::None: statisticsScope = EngineStatisticsScopeEnum::Full; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling scope enum");
						}
						return true;
					}
					if (key == keyToggleStatisticsMode)
					{
						switch (statisticsMode)
						{
						case EngineStatisticsModeEnum::Average: statisticsMode = EngineStatisticsModeEnum::Maximum; break;
						case EngineStatisticsModeEnum::Maximum: statisticsMode = EngineStatisticsModeEnum::Last; break;
						case EngineStatisticsModeEnum::Last: statisticsMode = EngineStatisticsModeEnum::Average; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling mode enum");
						}
						return true;
					}
					if (key == keyToggleProfilingEnabled)
					{
						confProfilingEnabled = !confProfilingEnabled;
						return true;
					}
					if (key == keyVisualizeBufferPrev)
					{
						confVisualizeBuffer = confVisualizeBuffer - 1;
						return true;
					}
					if (key == keyVisualizeBufferNext)
					{
						confVisualizeBuffer = confVisualizeBuffer + 1;
						return true;
					}
					if (key == keyToggleRenderMissingModels)
					{
						confRenderMissingModels = !confRenderMissingModels;
						return true;
					}
					if (key == keyToggleRenderSkeletonBones)
					{
						confRenderSkeletonBones = !confRenderSkeletonBones;
						return true;
					}
				}
				return false;
			}
		};
	}

	Holder<EngineStatistics> newEngineStatistics()
	{
		return systemMemory().createImpl<EngineStatistics, EngineStatisticsImpl>();
	}
}
