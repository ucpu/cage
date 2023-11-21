#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-engine/guiBuilder.h>
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
			const EventListener<bool(const GenericInput &)> keyPressListener = engineWindow()->events.listen(inputListener<InputClassEnum::KeyPress, InputKey>([this](InputKey in) { return this->keyPress(in); }));
			const EventListener<bool(const GenericInput &)> keyRepeatListener = engineWindow()->events.listen(inputListener<InputClassEnum::KeyRepeat, InputKey>([this](InputKey in) { return this->keyRepeat(in); }));
			const EventListener<bool()> updateListener = controlThread().update.listen([this]() { this->update(); });

			uint32 rootName = 0;
			bool needRegen = true;

			template<StatisticsGuiFlags Flags>
			void generate(GuiBuilder *g, const String &name)
			{
				g->label().text(name);
				g->label()
					.text("")
					.textAlign(TextAlignEnum::Right)
					.update(
						[this](Entity *e)
						{
							const uint64 v = engineStatisticsValues(Flags, this->statisticsMode);
							if constexpr (Flags <= StatisticsGuiFlags::FrameTime)
								e->value<GuiTextComponent>().value = Stringizer() + (v / 1000) + " ms";
							else
								e->value<GuiTextComponent>().value = Stringizer() + v;
						});
			}

			void update()
			{
				if (statisticsScope != StatisticsGuiScopeEnum::None && (rootName == 0 || !engineGuiEntities()->has(rootName)))
					needRegen = true;
				if (statisticsScope == StatisticsGuiScopeEnum::None && rootName != 0)
					needRegen = true;
				if (!needRegen)
					return;
				needRegen = false;

				if (rootName != 0 && engineGuiEntities()->has(rootName))
					detail::guiDestroyEntityRecursively(engineGuiEntities()->get(rootName));
				rootName = 0;

				if (statisticsScope == StatisticsGuiScopeEnum::None)
					return;

				rootName = engineGuiEntities()->createUnique()->name();
				Holder<GuiBuilder> g = newGuiBuilder(engineGuiEntities()->get(rootName));
				auto _1 = g->alignment(screenPosition);
				auto _2 = g->panel().skin(2); // compact style
				auto _3 = g->verticalTable(2);

				switch (statisticsScope)
				{
					case StatisticsGuiScopeEnum::Fps:
						generate<StatisticsGuiFlags::FrameTime>(+g, "Frame time: ");
						break;
					case StatisticsGuiScopeEnum::Full:
						generate<StatisticsGuiFlags::Control>(+g, "Control: ");
						generate<StatisticsGuiFlags::Sound>(+g, "Sound: ");
						generate<StatisticsGuiFlags::GraphicsPrepare>(+g, "Graphics prepare: ");
						generate<StatisticsGuiFlags::GraphicsDispatch>(+g, "Graphics dispatch: ");
						generate<StatisticsGuiFlags::FrameTime>(+g, "Frame time: ");
						generate<StatisticsGuiFlags::DrawCalls>(+g, "Draw calls: ");
						generate<StatisticsGuiFlags::DrawPrimitives>(+g, "Draw primitives: ");
						generate<StatisticsGuiFlags::Entities>(+g, "Entities: ");
						break;
					default:
						break;
				}
			}

			bool keyPress(InputKey in)
			{
				if (in.mods == keyModifiers)
				{
					if (in.key == keyToggleStatisticsScope)
					{
						switch (statisticsScope)
						{
							case StatisticsGuiScopeEnum::Full:
								statisticsScope = StatisticsGuiScopeEnum::Fps;
								break;
							case StatisticsGuiScopeEnum::Fps:
								statisticsScope = StatisticsGuiScopeEnum::None;
								break;
							case StatisticsGuiScopeEnum::None:
								statisticsScope = StatisticsGuiScopeEnum::Full;
								break;
							default:
								CAGE_THROW_CRITICAL(Exception, "invalid engine profiling scope enum");
						}
						needRegen = true;
						return true;
					}
					if (in.key == keyToggleStatisticsMode)
					{
						switch (statisticsMode)
						{
							case StatisticsGuiModeEnum::Average:
								statisticsMode = StatisticsGuiModeEnum::Maximum;
								break;
							case StatisticsGuiModeEnum::Maximum:
								statisticsMode = StatisticsGuiModeEnum::Latest;
								break;
							case StatisticsGuiModeEnum::Latest:
								statisticsMode = StatisticsGuiModeEnum::Average;
								break;
							default:
								CAGE_THROW_CRITICAL(Exception, "invalid engine profiling mode enum");
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
