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
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels");
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones");
		ConfigBool confProfilingEnabled("cage/profiling/enabled");
		ConfigFloat confRenderGamma("cage/graphics/gamma");

		class StatisticsGuiImpl : public StatisticsGui
		{
		public:
			const EventListener<bool(const GenericInput &)> keyPressListener = engineWindow()->events.listen(inputFilter([this](input::KeyPress in) { return this->keyPress(in); }));
			const EventListener<bool(const GenericInput &)> keyRepeatListener = engineWindow()->events.listen(inputFilter([this](input::KeyRepeat in) { return this->keyRepeat(in); }));
			const EventListener<bool()> updateListener = controlThread().update.listen([this]() { this->update(); });

			uint32 rootName = 0;
			bool needRegen = true;

			template<StatisticsGuiFlags Flags>
			void generate(GuiBuilder *g, const String &name)
			{
				g->label().text(name);
				g->label()
					.text("")
					.update(
						[this](Entity *e)
						{
							const uint64 v = engineStatisticsValues(Flags, this->statisticsMode);
							if constexpr (Flags == StatisticsGuiFlags::DynamicResolution)
								e->value<GuiTextComponent>().value = engineDynamicResolution().enabled ? (Stringizer() + v + " %") : (Stringizer() + "off");
							else if constexpr (Flags == StatisticsGuiFlags::CpuUtilization)
								e->value<GuiTextComponent>().value = Stringizer() + v + " %";
							else if constexpr (Flags <= StatisticsGuiFlags::FrameTime)
								e->value<GuiTextComponent>().value = Stringizer() + (v / 1000) + " ms";
							else
								e->value<GuiTextComponent>().value = Stringizer() + v;
						})
					.textAlign(TextAlignEnum::Right);
			}

			void update()
			{
				if (statisticsScope != StatisticsGuiScopeEnum::None && (rootName == 0 || !engineGuiEntities()->exists(rootName)))
					needRegen = true;
				if (statisticsScope == StatisticsGuiScopeEnum::None && rootName != 0)
					needRegen = true;
				if (!needRegen)
					return;
				needRegen = false;

				if (rootName != 0 && engineGuiEntities()->exists(rootName))
					detail::guiDestroyEntityRecursively(engineGuiEntities()->get(rootName));
				rootName = 0;

				if (statisticsScope == StatisticsGuiScopeEnum::None)
					return;

				rootName = engineGuiEntities()->createUnique()->id();
				Entity *root = engineGuiEntities()->get(rootName);
				root->value<GuiParentComponent>().order = 9000;
				Holder<GuiBuilder> g = newGuiBuilder(root);
				auto _1 = g->alignment(screenPosition);
				auto _2 = g->panel();
				auto _3 = g->verticalTable(2);

				switch (statisticsScope)
				{
					case StatisticsGuiScopeEnum::Fps:
						generate<StatisticsGuiFlags::FrameTime>(+g, "Frame time: ");
						break;
					case StatisticsGuiScopeEnum::Full:
						generate<StatisticsGuiFlags::CpuUtilization>(+g, "CPU utilization: ");
						generate<StatisticsGuiFlags::DynamicResolution>(+g, "Dyn. resolution: ");
						generate<StatisticsGuiFlags::ControlThreadTime>(+g, "Control time: ");
						generate<StatisticsGuiFlags::SoundThreadTime>(+g, "Sound time: ");
						generate<StatisticsGuiFlags::PrepareThreadTime>(+g, "Prepare time: ");
						generate<StatisticsGuiFlags::GpuTime>(+g, "GPU time: ");
						generate<StatisticsGuiFlags::FrameTime>(+g, "Frame time: ");
						generate<StatisticsGuiFlags::DrawCalls>(+g, "Draw calls: ");
						generate<StatisticsGuiFlags::DrawPrimitives>(+g, "Draw primitives: ");
						generate<StatisticsGuiFlags::Entities>(+g, "Entities: ");
						break;
					default:
						break;
				}
			}

			bool keyPress(input::KeyPress in)
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

			bool keyRepeat(input::KeyRepeat in)
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
