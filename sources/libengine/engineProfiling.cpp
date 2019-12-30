#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/engine.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include <cage-engine/engineProfiling.h>

namespace cage
{
	EngineProfiling::EngineProfiling() :
		keyToggleProfilingScope(290), // f1
		keyToggleProfilingMode(291), // f2
		keyVisualizeBufferPrev(292), // f3
		keyVisualizeBufferNext(293), // f4
		keyToggleRenderMissingMeshes(294), // f5
		keyToggleRenderSkeletonBones(295), // f6
		keyToggleStereo(298), // f9
		keyModifiers(ModifiersFlags::Ctrl),
		profilingScope(EngineProfilingScopeEnum::Full),
		profilingMode(EngineProfilingModeEnum::Maximum),
		screenPosition(1, 0)
	{}

	namespace
	{
		ConfigSint32 visualizeBuffer("cage/graphics/visualizeBuffer");
		ConfigBool confRenderMissingMeshes("cage/graphics/renderMissingMeshes");
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones");

		class EngineProfilingImpl : public EngineProfiling
		{
		public:
			EventListener<bool(uint32, uint32, ModifiersFlags)> keyPressListener;
			EventListener<bool()> updateListener;

			uint32 panelIndex;
			uint32 layoutIndex;
			uint32 labelIndices[40];
			const EngineProfilingStatsFlags *labelFlags;
			const char **labelNames;
			uint32 labelsCount;
			EngineProfilingScopeEnum profilingModeOld;

			EngineProfilingImpl() : profilingModeOld(EngineProfilingScopeEnum::Full)
			{
				nullData();

				keyPressListener.bind<EngineProfilingImpl, &EngineProfilingImpl::keyPress>(this);
				updateListener.bind<EngineProfilingImpl, &EngineProfilingImpl::update>(this);

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
				static const EngineProfilingStatsFlags flagsFull[] = {
					EngineProfilingStatsFlags::Control,
					EngineProfilingStatsFlags::Sound,
					EngineProfilingStatsFlags::GraphicsPrepare,
					EngineProfilingStatsFlags::GraphicsDispatch,
					EngineProfilingStatsFlags::FrameTime,
					EngineProfilingStatsFlags::DrawCalls,
					EngineProfilingStatsFlags::DrawPrimitives,
					EngineProfilingStatsFlags::Entities,
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

				static const EngineProfilingStatsFlags flagsShort[] = {
					EngineProfilingStatsFlags::Control,
					EngineProfilingStatsFlags::Sound,
					EngineProfilingStatsFlags::GraphicsPrepare,
					EngineProfilingStatsFlags::FrameTime,
				};
				static const char* namesShort[sizeof(flagsShort) / sizeof(flagsShort[0])] = {
					"Control: ",
					"Sound: ",
					"Prepare: ",
					"Frame: ",
				};

				static const EngineProfilingStatsFlags flagsFps[] = {
					EngineProfilingStatsFlags::FrameTime,
				};
				static const char* namesFps[sizeof(flagsFps) / sizeof(flagsFps[0])] = {
					"Frame Time: ",
				};

				clearEntities();
				profilingModeOld = profilingScope;
				EntityManager *g = gui()->entities();
				Entity *panel = g->createUnique();
				{
					panelIndex = panel->name();
					CAGE_COMPONENT_GUI(Scrollbars, sc, panel);
					sc.alignment = screenPosition;
				}
				Entity *layout = g->createUnique();
				{
					layoutIndex = layout->name();
					CAGE_COMPONENT_GUI(Panel, c, layout);
					CAGE_COMPONENT_GUI(LayoutTable, l, layout);
					CAGE_COMPONENT_GUI(Parent, child, layout);
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
					Entity *e = g->createUnique();
					labelIndices[i] = e->name();
					CAGE_COMPONENT_GUI(Label, c, e);
					CAGE_COMPONENT_GUI(Parent, child, e);
					child.parent = layout->name();
					child.order = i;
					if (i % 2 == 1)
					{
						CAGE_COMPONENT_GUI(TextFormat, tf, e);
						tf.align = TextAlignEnum::Right;
					}
				}

				switch (profilingScope)
				{
				case EngineProfilingScopeEnum::None:
					labelFlags = nullptr;
					labelNames = nullptr;
					break;
				case EngineProfilingScopeEnum::Fps:
					labelFlags = flagsFps;
					labelNames = namesFps;
					break;
				case EngineProfilingScopeEnum::Short:
					labelFlags = flagsShort;
					labelNames = namesShort;
					break;
				case EngineProfilingScopeEnum::Full:
					labelFlags = flagsFull;
					labelNames = namesFull;
					break;
				}
			}

			void destroyEntity(uint32 name)
			{
				if (name == 0)
					return;
				EntityManager *g = gui()->entities();
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
				EntityManager *g = gui()->entities();
				bool panelPresent = panelIndex != 0 && g->has(panelIndex);
				bool visible = profilingScope != EngineProfilingScopeEnum::None;
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
				Entity *timing = gui()->entities()->get(labelIndices[index]);
				CAGE_COMPONENT_GUI(Text, t, timing);
				t.value = value;
			}

			bool update()
			{
				checkEntities();
				for (uint32 i = 0; i < labelsCount; i++)
				{
					setTextLabel(i * 2 + 0, labelNames[i]);
					if (labelFlags[i] <= EngineProfilingStatsFlags::FrameTime)
						setTextLabel(i * 2 + 1, stringizer() + (engineProfilingValues(labelFlags[i], profilingMode) / 1000) + " ms");
					else
						setTextLabel(i * 2 + 1, string(engineProfilingValues(labelFlags[i], profilingMode)));
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
						window()->setFullscreen(ivec2(0, 0));
					}
					catch (...)
					{
						setWindowFullscreen(false);
					}
				}
				else
				{
					window()->setWindowed(WindowFlags::Resizeable | WindowFlags::Border);
				}
			}

			bool keyPress(uint32 key, uint32, ModifiersFlags mods)
			{
				if (mods == keyModifiers)
				{
					if (key == keyToggleProfilingScope)
					{
						switch (profilingScope)
						{
						case EngineProfilingScopeEnum::Full: profilingScope = EngineProfilingScopeEnum::Short; break;
						case EngineProfilingScopeEnum::Short: profilingScope = EngineProfilingScopeEnum::Fps; break;
						case EngineProfilingScopeEnum::Fps: profilingScope = EngineProfilingScopeEnum::None; break;
						case EngineProfilingScopeEnum::None: profilingScope = EngineProfilingScopeEnum::Full; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling scope enum");
						}
						return true;
					}
					if (key == keyToggleProfilingMode)
					{
						switch (profilingMode)
						{
						case EngineProfilingModeEnum::Average: profilingMode = EngineProfilingModeEnum::Maximum; break;
						case EngineProfilingModeEnum::Maximum: profilingMode = EngineProfilingModeEnum::Last; break;
						case EngineProfilingModeEnum::Last: profilingMode = EngineProfilingModeEnum::Average; break;
						default: CAGE_THROW_CRITICAL(Exception, "invalid engine profiling mode enum");
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
						case StereoModeEnum::Mono:
							graphicsPrepareThread().stereoMode = StereoModeEnum::Horizontal;
							break;
						case StereoModeEnum::Horizontal:
							graphicsPrepareThread().stereoMode = StereoModeEnum::Vertical;
							break;
						case StereoModeEnum::Vertical:
							graphicsPrepareThread().stereoMode = StereoModeEnum::Mono;
							break;
						}
						return true;
					}
					if (key == keyToggleRenderMissingMeshes)
					{
						confRenderMissingMeshes = !confRenderMissingMeshes;
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

	Holder<EngineProfiling> newEngineProfiling()
	{
		return detail::systemArena().createImpl<EngineProfiling, EngineProfilingImpl>();
	}
}
