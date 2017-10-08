#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/engine.h>
#include <cage-client/graphic.h>
#include <cage-client/utility/engineProfiling.h>

namespace cage
{
	namespace
	{
		class engineProfilingImpl : public engineProfilingClass
		{
		public:
			eventListener<bool(windowClass *w, uint32 a, uint32 b, modifiersFlags m)> keyReleaseListener;
			eventListener<bool(uint64)> updateListener;

			uint32 panelIndex;
			uint32 layoutIndex;
			uint32 labelIndices[20];
			const engineProfiling::profilingTimeFlags *labelFlags;
			const char **labelNames;
			uint32 labelsCount;
			profilingModeEnum profilingModeOld;

			engineProfilingImpl() : profilingModeOld(profilingModeEnum::Full)
			{
				nullData();
				keyToggleProfilingMode = 290; // f1
				keyVisualizeBufferPrev = 291; // f2
				keyVisualizeBufferNext = 292; // f3
				keyToggleRenderMissingMeshes = 293; // f4
				keyToggleStereo = 294; // f5
				keyToggleFullscreen = 300; // f11
				profilingMode = profilingModeEnum::Full;
				screenPosition = vec2(1, 0);

				keyReleaseListener.bind<engineProfilingImpl, &engineProfilingImpl::keyRelease>(this);
				updateListener.bind<engineProfilingImpl, &engineProfilingImpl::update>(this);

				window()->events.keyRelease.add(keyReleaseListener);
				controlThread::update.add(updateListener);
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
				clearEntities();
				profilingModeOld = profilingMode;
				entityManagerClass *g = gui()->entities();
				entityClass *panel = g->newEntity(panelIndex = g->generateUniqueName());
				{
					GUI_GET_COMPONENT(control, c, panel);
					c.controlType = controlTypeEnum::Panel;
					GUI_GET_COMPONENT(position, p, panel);
					p.anchorX = screenPosition[0];
					p.anchorY = screenPosition[1];
					p.x = screenPosition[0];
					p.y = screenPosition[1];
					p.xUnit = unitsModeEnum::ScreenWidth;
					p.yUnit = unitsModeEnum::ScreenHeight;
				}
				entityClass *layout = g->newEntity(layoutIndex = g->generateUniqueName());
				{
					GUI_GET_COMPONENT(layout, l, layout);
					l.layoutMode = layoutModeEnum::Column;
					GUI_GET_COMPONENT(parent, child, layout);
					child.parent = panel->getName();
				}

				static const uint32 labelsPerMode[] = { 14, 4, 1, 0 };
				labelsCount = labelsPerMode[(uint32)profilingMode];
				for (uint32 i = 0; i < labelsCount + (profilingMode == profilingModeEnum::Full ? 1 : 0); i++)
				{
					entityClass *timing = g->newEntity(labelIndices[i] = g->generateUniqueName());
					GUI_GET_COMPONENT(control, c, timing);
					c.controlType = controlTypeEnum::Empty;
					GUI_GET_COMPONENT(parent, child, timing);
					child.parent = layout->getName();
					child.ordering = i;
				}

				static const engineProfiling::profilingTimeFlags flagsFull[] = {
					engineProfiling::profilingTimeFlags::ControlTick,
					engineProfiling::profilingTimeFlags::ControlWait,
					engineProfiling::profilingTimeFlags::ControlEmit,
					engineProfiling::profilingTimeFlags::ControlSleep,
					engineProfiling::profilingTimeFlags::GraphicPrepareWait,
					engineProfiling::profilingTimeFlags::GraphicPrepareEmit,
					engineProfiling::profilingTimeFlags::GraphicPrepareTick,
					engineProfiling::profilingTimeFlags::GraphicDispatchWait,
					engineProfiling::profilingTimeFlags::GraphicDispatchTick,
					engineProfiling::profilingTimeFlags::GraphicDispatchSwap,
					engineProfiling::profilingTimeFlags::SoundEmit,
					engineProfiling::profilingTimeFlags::SoundTick,
					engineProfiling::profilingTimeFlags::SoundSleep,
					engineProfiling::profilingTimeFlags::FrameTime
				};
				static const char* namesFull[] = {
					"Control Tick",
					"Control Wait",
					"Control Emit",
					"Control Sleep",
					"Graphic Prepare Wait",
					"Graphic Prepare Emit",
					"Graphic Prepare Tick",
					"Graphic Dispatch Wait",
					"Graphic Dispatch Tick",
					"Graphic Dispatch Swap",
					"Sound Emit",
					"Sound Tick",
					"Sound Sleep",
					"Frame Time"
				};

				static const engineProfiling::profilingTimeFlags flagsShort[] = {
					(engineProfiling::profilingTimeFlags)(engineProfiling::profilingTimeFlags::ControlTick),
					(engineProfiling::profilingTimeFlags)(engineProfiling::profilingTimeFlags::GraphicPrepareTick | engineProfiling::profilingTimeFlags::GraphicPrepareEmit),
					(engineProfiling::profilingTimeFlags)(engineProfiling::profilingTimeFlags::GraphicDispatchTick | engineProfiling::profilingTimeFlags::GraphicDispatchSwap),
					(engineProfiling::profilingTimeFlags)(engineProfiling::profilingTimeFlags::SoundTick | engineProfiling::profilingTimeFlags::SoundEmit),
				};
				static const char* namesShort[] = {
					"Control",
					"Prepare",
					"Dispatch",
					"Sound",
				};

				static const engineProfiling::profilingTimeFlags flagsFps[] = {
					engineProfiling::profilingTimeFlags::FrameTime,
				};
				static const char* namesFps[] = {
					"Frame Time",
				};

				switch (profilingMode)
				{
				case profilingModeEnum::None:
					labelFlags = nullptr;
					labelNames = nullptr;
					break;
				case profilingModeEnum::Fps:
					labelFlags = flagsFps;
					labelNames = namesFps;
					break;
				case profilingModeEnum::Short:
					labelFlags = flagsShort;
					labelNames = namesShort;
					break;
				case profilingModeEnum::Full:
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
				if (!g->hasEntity(name))
					return;
				g->getEntity(name)->destroy();
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
				bool panelPresent = panelIndex != 0 && g->hasEntity(panelIndex);
				bool visible = profilingMode != profilingModeEnum::None;
				if (panelPresent != visible || profilingModeOld != profilingMode)
				{ // change needed
					clearEntities();
					if (visible)
						generateEntities();
				}
			}

			void setTextLabel(uint32 index, const string &value)
			{
				CAGE_ASSERT_RUNTIME(index < sizeof(labelIndices) / sizeof(labelIndices[0]), index);
				if (labelIndices[index] == 0 || !gui()->entities()->hasEntity(labelIndices[index]))
					return;
				entityClass *timing = gui()->entities()->getEntity(labelIndices[index]);
				GUI_GET_COMPONENT(text, t, timing);
				t.value = value;
			}

			void setTimingLabel(uint32 index, const string &name, uint64 value)
			{
				setTextLabel(index, name + ": " + (value / 1000) + " ms");
			}

			bool update(uint64)
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
					setTimingLabel(i, labelNames[i], engineProfiling::getTime(labelFlags[i], true));
				if (profilingModeOld == profilingModeEnum::Full)
					setTextLabel(labelsCount, string() + "entities: " + entities()->getAllEntities()->entitiesCount());
				return false;
			}

			void setWindowMode(bool fullscreen)
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
						setWindowMode(false);
					}
				}
				else
				{
					window()->modeSetWindowed((windowFlags)(windowFlags::Resizeable | windowFlags::Border));
					window()->windowedSize(pointStruct(800, 600));
				}
			}

			bool keyRelease(windowClass *w, uint32 key, uint32 scanCode, modifiersFlags m)
			{
				static configSint32 visualizeBuffer("cage-client.engine.debugVisualizeBuffer");
				static configBool renderMeshes("cage-client.engine.debugRenderMissingMeshes");

				if (key == keyToggleProfilingMode)
				{
					switch (profilingMode)
					{
					case profilingModeEnum::Full: profilingMode = profilingModeEnum::Short; break;
					case profilingModeEnum::Short: profilingMode = profilingModeEnum::Fps; break;
					case profilingModeEnum::Fps: profilingMode = profilingModeEnum::None; break;
					case profilingModeEnum::None: profilingMode = profilingModeEnum::Full; break;
					}
				}
				if (key == keyVisualizeBufferPrev)
					visualizeBuffer = visualizeBuffer - 1;
				if (key == keyVisualizeBufferNext)
					visualizeBuffer = visualizeBuffer + 1;
				if (key == keyToggleStereo)
				{
					switch (graphicPrepareThread::stereoMode)
					{
					case stereoModeEnum::Mono:
						graphicPrepareThread::stereoMode = stereoModeEnum::LeftRight;
						break;
					case stereoModeEnum::LeftRight:
						graphicPrepareThread::stereoMode = stereoModeEnum::TopBottom;
						break;
					case stereoModeEnum::TopBottom:
						graphicPrepareThread::stereoMode = stereoModeEnum::Mono;
						break;
					}
				}
				if (key == keyToggleRenderMissingMeshes)
					renderMeshes = !renderMeshes;
				if (key == keyToggleFullscreen)
					setWindowMode(!window()->isFullscreen());
				return false;
			}
		};
	}

	holder<engineProfilingClass> newEngineProfiling()
	{
		return detail::systemArena().createImpl<engineProfilingClass, engineProfilingImpl>();
	}
}