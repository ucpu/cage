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
	engineProfiling::engineProfiling() :
		keyToggleProfilingScope(290), // f1
		keyToggleProfilingMode(291), // f2
		keyVisualizeBufferPrev(292), // f3
		keyVisualizeBufferNext(293), // f4
		keyToggleRenderMissingMeshes(294), // f5
		keyToggleRenderSkeletonBones(295), // f6
		keyToggleStereo(298), // f9
		keyToggleFullscreen(300), // f11
		keyModifiers(modifiersFlags::Ctrl),
		profilingScope(engineProfilingScopeEnum::Full),
		profilingMode(engineProfilingModeEnum::Maximum),
		screenPosition(1, 0)
	{}

	namespace
	{
		configSint32 visualizeBuffer("cage-client.engine.debugVisualizeBuffer");
		configBool renderMissingMeshes("cage-client.engine.debugMissingMeshes");
		configBool renderSkeletonBones("cage-client.engine.debugSkeletonBones");

		class engineProfilingImpl : public engineProfiling
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
					engineProfilingStatsFlags::Control,
					engineProfilingStatsFlags::Sound,
					engineProfilingStatsFlags::GraphicsPrepare,
					engineProfilingStatsFlags::GraphicsDispatch,
					engineProfilingStatsFlags::FrameTime,
					engineProfilingStatsFlags::DrawCalls,
					engineProfilingStatsFlags::DrawPrimitives,
					engineProfilingStatsFlags::Entities,
				};
				static const char* namesFull[sizeof(flagsFull) / sizeof(flagsFull[0])] = {
					"Control: ",
					"Sound: ",
					"Graphics Prepare: ",
					"Graphics Dispatch: ",
					"Frame Time: ",
					"Draw Calls: ",
					"Draw Primitives: ",
					"Entities: ",
				};

				static const engineProfilingStatsFlags flagsShort[] = {
					engineProfilingStatsFlags::Control,
					engineProfilingStatsFlags::Sound,
					engineProfilingStatsFlags::GraphicsPrepare,
					engineProfilingStatsFlags::FrameTime,
				};
				static const char* namesShort[sizeof(flagsShort) / sizeof(flagsShort[0])] = {
					"Control: ",
					"Sound: ",
					"Prepare: ",
					"Frame: ",
				};

				static const engineProfilingStatsFlags flagsFps[] = {
					engineProfilingStatsFlags::FrameTime,
				};
				static const char* namesFps[sizeof(flagsFps) / sizeof(flagsFps[0])] = {
					"Frame Time: ",
				};

				clearEntities();
				profilingModeOld = profilingScope;
				entityManager *g = gui()->entities();
				entity *panel = g->createUnique();
				{
					panelIndex = panel->name();
					CAGE_COMPONENT_GUI(scrollbars, sc, panel);
					sc.alignment = screenPosition;
				}
				entity *layout = g->createUnique();
				{
					layoutIndex = layout->name();
					CAGE_COMPONENT_GUI(panel, c, layout);
					CAGE_COMPONENT_GUI(layoutTable, l, layout);
					CAGE_COMPONENT_GUI(parent, child, layout);
					child.parent = panel->name();
				}

				static const uint32 labelsPerMode[] = {
					sizeof(flagsFull) / sizeof(flagsFull[0]),
					sizeof(flagsShort) / sizeof(flagsShort[0]),
					sizeof(flagsFps) / sizeof(flagsFps[0]),
					0 };
				labelsCount = labelsPerMode[(uint32)profilingScope];
				for (uint32 i = 0; i < labelsCount * 2; i++)
				{
					entity *e = g->createUnique();
					labelIndices[i] = e->name();
					CAGE_COMPONENT_GUI(label, c, e);
					CAGE_COMPONENT_GUI(parent, child, e);
					child.parent = layout->name();
					child.order = i;
					if (i % 2 == 1)
					{
						CAGE_COMPONENT_GUI(textFormat, tf, e);
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
				entityManager *g = gui()->entities();
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
				entityManager *g = gui()->entities();
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
				CAGE_ASSERT(index < sizeof(labelIndices) / sizeof(labelIndices[0]), index);
				if (labelIndices[index] == 0 || !gui()->entities()->has(labelIndices[index]))
					return;
				entity *timing = gui()->entities()->get(labelIndices[index]);
				CAGE_COMPONENT_GUI(text, t, timing);
				t.value = value;
			}

			bool update()
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
				{
					setTextLabel(i * 2 + 0, labelNames[i]);
					if (labelFlags[i] <= engineProfilingStatsFlags::FrameTime)
						setTextLabel(i * 2 + 1, string() + (engineProfilingValues(labelFlags[i], profilingMode) / 1000) + " ms");
					else
						setTextLabel(i * 2 + 1, engineProfilingValues(labelFlags[i], profilingMode));
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
						window()->setFullscreen(ivec2(0, 0));
					}
					catch (...)
					{
						setWindowFullscreen(false);
					}
				}
				else
				{
					window()->setWindowed(windowFlags::Resizeable | windowFlags::Border);
				}
			}

			bool keyPress(uint32 key, uint32, modifiersFlags mods)
			{
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
						renderMissingMeshes = !renderMissingMeshes;
						return true;
					}
					if (key == keyToggleRenderSkeletonBones)
					{
						renderSkeletonBones = !renderSkeletonBones;
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

	holder<engineProfiling> newEngineProfiling()
	{
		return detail::systemArena().createImpl<engineProfiling, engineProfilingImpl>();
	}
}
