#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include <cage-client/engineProfiling.h>

namespace cage
{
	namespace
	{
		class engineProfilingImpl : public engineProfilingClass
		{
		public:
			eventListener<bool(uint32, uint32, modifiersFlags)> keyPressListener;
			eventListener<bool()> updateListener;

			uint32 panelIndex;
			uint32 layoutIndex;
			uint32 labelIndices[40];
			const engineProfilingStatsFlags *labelFlags;
			const char **labelNames;
			uint32 labelsCount;
			engineProfilingScopeEnum profilingModeOld;

			engineProfilingImpl() : profilingModeOld(engineProfilingScopeEnum::Full)
			{
				nullData();
				keyToggleProfilingScope = 290; // f1
				keyToggleProfilingMode = 291; // f2
				keyVisualizeBufferPrev = 294; // f5
				keyVisualizeBufferNext = 295; // f6
				keyToggleRenderMissingMeshes = 297; // f8
				keyToggleStereo = 298; // f9
				keyToggleFullscreen = 300; // f11
				keyModifiers = modifiersFlags::Ctrl;
				profilingScope = engineProfilingScopeEnum::Full;
				profilingMode = engineProfilingModeEnum::Average;
				screenPosition = vec2(1, 0);

				keyPressListener.bind<engineProfilingImpl, &engineProfilingImpl::keyPress>(this);
				updateListener.bind<engineProfilingImpl, &engineProfilingImpl::update>(this);

				window()->events.keyPress.attach(keyPressListener);
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
				static const engineProfilingStatsFlags flagsFull[] = {
					engineProfilingStatsFlags::ControlTick,
					engineProfilingStatsFlags::ControlEmit,
					engineProfilingStatsFlags::ControlSleep,
					engineProfilingStatsFlags::GraphicsPrepareWait,
					engineProfilingStatsFlags::GraphicsPrepareTick,
					engineProfilingStatsFlags::GraphicsDispatchWait,
					engineProfilingStatsFlags::GraphicsDispatchTick,
					engineProfilingStatsFlags::GraphicsDispatchSwap,
					engineProfilingStatsFlags::SoundWait,
					engineProfilingStatsFlags::SoundTick,
					engineProfilingStatsFlags::SoundSleep,
					engineProfilingStatsFlags::FrameTime
				};
				static const char* namesFull[sizeof(flagsFull) / sizeof(flagsFull[0])] = {
					"Control Tick: ",
					"Control Emit: ",
					"Control Sleep: ",
					"Graphics Prepare Wait: ",
					"Graphics Prepare Tick: ",
					"Graphics Dispatch Wait: ",
					"Graphics Dispatch Tick: ",
					"Graphics Dispatch Swap: ",
					"Sound Wait: ",
					"Sound Tick: ",
					"Sound Sleep: ",
					"Frame Time: "
				};

				static const engineProfilingStatsFlags flagsShort[] = {
					engineProfilingStatsFlags::ControlTick | engineProfilingStatsFlags::ControlEmit,
					engineProfilingStatsFlags::GraphicsPrepareTick,
					engineProfilingStatsFlags::GraphicsDispatchTick,
					engineProfilingStatsFlags::SoundTick,
				};
				static const char* namesShort[sizeof(flagsShort) / sizeof(flagsShort[0])] = {
					"Control: ",
					"Prepare: ",
					"Dispatch: ",
					"Sound: ",
				};

				static const engineProfilingStatsFlags flagsFps[] = {
					engineProfilingStatsFlags::FrameTime,
				};
				static const char* namesFps[sizeof(flagsFps) / sizeof(flagsFps[0])] = {
					"Frame Time: ",
				};

				clearEntities();
				profilingModeOld = profilingScope;
				entityManagerClass *g = gui()->entities();
				entityClass *panel = g->createUnique();
				{
					panelIndex = panel->name();
					GUI_GET_COMPONENT(scrollbars, sc, panel);
					sc.alignment = screenPosition;
				}
				entityClass *layout = g->createUnique();
				{
					layoutIndex = layout->name();
					GUI_GET_COMPONENT(panel, c, layout);
					GUI_GET_COMPONENT(layoutTable, l, layout);
					GUI_GET_COMPONENT(parent, child, layout);
					child.parent = panel->name();
				}

				static const uint32 labelsPerMode[] = {
					sizeof(flagsFull) / sizeof(flagsFull[0]),
					sizeof(flagsShort) / sizeof(flagsShort[0]),
					sizeof(flagsFps) / sizeof(flagsFps[0]),
					0 };
				labelsCount = labelsPerMode[(uint32)profilingScope];
				uint32 labelsCountExt = labelsCount + (profilingScope == engineProfilingScopeEnum::Full ? 3 : 0);
				for (uint32 i = 0; i < labelsCountExt * 2; i++)
				{
					entityClass *timing = g->createUnique();
					labelIndices[i] = timing->name();
					GUI_GET_COMPONENT(label, c, timing);
					GUI_GET_COMPONENT(parent, child, timing);
					child.parent = layout->name();
					child.order = i;
					if (i % 2 == 1)
					{
						GUI_GET_COMPONENT(textFormat, tf, timing);
						tf.align = textAlignEnum::Right;
					}
				}

				switch (profilingScope)
				{
				case engineProfilingScopeEnum::None:
					labelFlags = nullptr;
					labelNames = nullptr;
					break;
				case engineProfilingScopeEnum::Fps:
					labelFlags = flagsFps;
					labelNames = namesFps;
					break;
				case engineProfilingScopeEnum::Short:
					labelFlags = flagsShort;
					labelNames = namesShort;
					break;
				case engineProfilingScopeEnum::Full:
					labelFlags = flagsFull;
					labelNames = namesFull;
					break;
				}
			}

			void destroyEntity(uint32 name)
			{
				if (name == 0)
					return;
				entityManagerClass *g = gui()->entities();
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
				entityManagerClass *g = gui()->entities();
				bool panelPresent = panelIndex != 0 && g->has(panelIndex);
				bool visible = profilingScope != engineProfilingScopeEnum::None;
				if (panelPresent != visible || profilingModeOld != profilingScope)
				{ // change needed
					clearEntities();
					if (visible)
						generateEntities();
				}
			}

			void setTextLabel(uint32 index, const string &value)
			{
				CAGE_ASSERT_RUNTIME(index < sizeof(labelIndices) / sizeof(labelIndices[0]), index);
				if (labelIndices[index] == 0 || !gui()->entities()->has(labelIndices[index]))
					return;
				entityClass *timing = gui()->entities()->get(labelIndices[index]);
				GUI_GET_COMPONENT(text, t, timing);
				t.value = value;
			}

			bool update()
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
				{
					setTextLabel(i * 2 + 0, labelNames[i]);
					setTextLabel(i * 2 + 1, string() + (engineProfilingValues(labelFlags[i], profilingMode) / 1000) + " ms");
				}
				if (profilingModeOld == engineProfilingScopeEnum::Full)
				{
					setTextLabel(labelsCount * 2 + 0, "Entities: ");
					setTextLabel(labelsCount * 2 + 1, entities()->group()->count());
					setTextLabel(labelsCount * 2 + 2, "Draw Calls: ");
					setTextLabel(labelsCount * 2 + 3, engineProfilingValues(engineProfilingStatsFlags::GraphicsDrawCalls, profilingMode));
					setTextLabel(labelsCount * 2 + 4, "Draw Primitives: ");
					setTextLabel(labelsCount * 2 + 5, engineProfilingValues(engineProfilingStatsFlags::GraphicsDrawPrimitives, profilingMode));
				}
				return false;
			}

			void setWindowFullscreen(bool fullscreen)
			{
				if (fullscreen)
				{
					try
					{
						detail::overrideBreakpoint ob;
						window()->modeSetFullscreen(pointStruct(0, 0));
					}
					catch (...)
					{
						setWindowFullscreen(false);
					}
				}
				else
				{
					window()->modeSetWindowed((windowFlags)(windowFlags::Resizeable | windowFlags::Border));
					window()->windowedSize(pointStruct(800, 600));
				}
			}

			bool keyPress(uint32 key, uint32, modifiersFlags mods)
			{
				static configSint32 visualizeBuffer("cage-client.engine.visualizeBuffer");
				static configBool renderMeshes("cage-client.engine.renderMissingMeshes");

				if (mods == keyModifiers)
				{
					if (key == keyToggleProfilingScope)
					{
						switch (profilingScope)
						{
						case engineProfilingScopeEnum::Full: profilingScope = engineProfilingScopeEnum::Short; break;
						case engineProfilingScopeEnum::Short: profilingScope = engineProfilingScopeEnum::Fps; break;
						case engineProfilingScopeEnum::Fps: profilingScope = engineProfilingScopeEnum::None; break;
						case engineProfilingScopeEnum::None: profilingScope = engineProfilingScopeEnum::Full; break;
						default: CAGE_THROW_CRITICAL(exception, "invalid engine profiling scope enum");
						}
						return true;
					}
					if (key == keyToggleProfilingMode)
					{
						switch (profilingMode)
						{
						case engineProfilingModeEnum::Average: profilingMode = engineProfilingModeEnum::Maximum; break;
						case engineProfilingModeEnum::Maximum: profilingMode = engineProfilingModeEnum::Last; break;
						case engineProfilingModeEnum::Last: profilingMode = engineProfilingModeEnum::Average; break;
						default: CAGE_THROW_CRITICAL(exception, "invalid engine profiling mode enum");
						}
						return true;
					}
					if (key == keyVisualizeBufferPrev)
					{
						visualizeBuffer = visualizeBuffer - 1;
						return true;
					}
					if (key == keyVisualizeBufferNext)
					{
						visualizeBuffer = visualizeBuffer + 1;
						return true;
					}
					if (key == keyToggleStereo)
					{
						switch (graphicsPrepareThread().stereoMode)
						{
						case stereoModeEnum::Mono:
							graphicsPrepareThread().stereoMode = stereoModeEnum::LeftRight;
							break;
						case stereoModeEnum::LeftRight:
							graphicsPrepareThread().stereoMode = stereoModeEnum::TopBottom;
							break;
						case stereoModeEnum::TopBottom:
							graphicsPrepareThread().stereoMode = stereoModeEnum::Mono;
							break;
						}
						return true;
					}
					if (key == keyToggleRenderMissingMeshes)
					{
						renderMeshes = !renderMeshes;
						return true;
					}
					if (key == keyToggleFullscreen)
					{
						setWindowFullscreen(!window()->isFullscreen());
						return true;
					}
				}
				return false;
			}
		};
	}

	holder<engineProfilingClass> newEngineProfiling()
	{
		return detail::systemArena().createImpl<engineProfilingClass, engineProfilingImpl>();
	}
}
