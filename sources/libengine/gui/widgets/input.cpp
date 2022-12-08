#include <cage-core/utf.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>
#include "../private.h"

namespace cage
{
	void textCreate(HierarchyItem *item);

	namespace
	{
		struct InputImpl : public WidgetItem
		{
			GuiInputComponent &data;
			GuiSelectionComponent &selection;
			bool showArrows = false;

			Vec2 leftPos, rightPos;
			Vec2 mainPos, mainSize;
			Vec2 textPos, textSize;

			InputImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Input)), selection(GUI_REF_COMPONENT(Selection))
			{}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->image);

				showArrows = data.type == InputTypeEnum::Real || data.type == InputTypeEnum::Integer;

				switch (data.type)
				{
				case InputTypeEnum::Integer:
					CAGE_ASSERT(data.step.i >= 0);
					CAGE_ASSERT(data.max.i >= data.min.i);
					break;
				case InputTypeEnum::Real:
					CAGE_ASSERT(data.min.f.valid() && data.step.f.valid() && data.max.f.valid());
					CAGE_ASSERT(data.step.f >= 0);
					CAGE_ASSERT(data.max.f >= data.min.f);
					break;
				default:
					break;
				}

				if (hasFocus())
					validate();
				else
					consolidate();

				if (!hierarchy->text)
					textCreate(hierarchy);
				hierarchy->text->skipInitialize = true;
				if (data.value.empty() && !hasFocus())
				{ // placeholder
					hierarchy->text->apply(skin->defaults.inputBox.placeholderFormat);
					hierarchy->text->transcript();
				}
				else
				{ // actual value
					if (data.valid)
						hierarchy->text->apply(skin->defaults.inputBox.textValidFormat);
					else
						hierarchy->text->apply(skin->defaults.inputBox.textInvalidFormat);
					if (data.type == InputTypeEnum::Password)
					{
						hierarchy->text->transcript(String("*"));
						const uint32 g = hierarchy->text->glyphs[0];
						hierarchy->text->transcript(data.value);
						for (uint32 &it : hierarchy->text->glyphs)
							it = g;
					}
					else
						hierarchy->text->transcript(data.value);
				}

				if (hasFocus())
				{
					data.cursor = min(data.cursor, utf32Length(data.value));
					hierarchy->text->cursor = data.cursor;
				}
			}

			template<class T>
			T consolidate(T value, T a, T b, T s)
			{
				if ((data.style & InputStyleFlags::AlwaysRoundValueToStep) == InputStyleFlags::AlwaysRoundValueToStep && (value % s) != 0)
					value -= value % s;
				return clamp(value, a, b);
			}

			void consolidate()
			{
				try
				{
					detail::OverrideBreakpoint ob;
					switch (data.type)
					{
					case InputTypeEnum::Integer:
						if (isInteger(data.value))
							data.value = Stringizer() + consolidate<sint32>(toSint32(data.value), data.min.i, data.max.i, data.step.i);
						break;
					case InputTypeEnum::Real:
						if (isReal(data.value))
							data.value = Stringizer() + consolidate<Real>(toFloat(data.value), data.min.f, data.max.f, data.step.f);
						break;
					default:
						break;
					}
				}
				catch (...)
				{
					data.value = "";
				}
				validate();
			}

			void validate()
			{
				try
				{
					detail::OverrideBreakpoint ob;
					switch (data.type)
					{
					case InputTypeEnum::Text:
						data.valid = true;
						break;
					case InputTypeEnum::Integer:
					{
						if (isInteger(data.value))
						{
							sint32 v = toSint32(data.value);
							sint32 r = consolidate<sint32>(v, data.min.i, data.max.i, data.step.i);
							data.valid = v == r;
						}
						else
							data.valid = false;
					} break;
					case InputTypeEnum::Real:
					{
						if (isReal(data.value))
						{
							Real v = toFloat(data.value);
							Real r = consolidate<Real>(v, data.min.f, data.max.f, data.step.f);
							data.valid = abs(v - r) < 1e-7;
						}
						else
							data.valid = false;
					} break;
					case InputTypeEnum::Email:
						// todo
						break;
					case InputTypeEnum::Url:
						// todo
						break;
					default:
						data.valid = !data.value.empty();
						break;
					}
				}
				catch (...)
				{
					data.valid = false;
				}
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.inputBox.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.inputBox.margin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				const auto &s = skin->defaults.inputBox;
				mainPos = hierarchy->renderPos;
				mainSize = hierarchy->renderSize;
				offset(mainPos, mainSize, -s.margin);
				leftPos = rightPos = mainPos;
				if (showArrows)
				{
					mainSize[0] -= (s.buttonsWidth + s.buttonsOffset) * 2;
					Real mw = mainSize[0];
					Real bw = s.buttonsWidth;
					Real off = s.buttonsOffset;
					Real bwo = bw + off;
					switch (s.buttonsMode)
					{
					case InputButtonsPlacementModeEnum::Left:
						rightPos[0] += bwo;
						mainPos[0] += bwo * 2;
						break;
					case InputButtonsPlacementModeEnum::Right:
						leftPos[0] += mw + off;
						rightPos[0] += mw + bwo + off;
						break;
					case InputButtonsPlacementModeEnum::Sides:
						mainPos[0] += bwo;
						rightPos[0] += bwo + mw + off;
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid input buttons placement enum");
					}
				}
				textPos = mainPos;
				textSize = mainSize;
				offset(textPos, textSize, -skin->layouts[(uint32)GuiElementTypeEnum::Input].border - s.basePadding);
			}

			void emit() override
			{
				const auto &s = skin->defaults.inputBox;
				emitElement(GuiElementTypeEnum::Input, mode(mainPos, mainSize), mainPos, mainSize);
				if (showArrows)
				{
					Vec2 ss = Vec2(s.buttonsWidth, mainSize[1]);
					ElementModeEnum m = mode(leftPos, ss);
					emitElement(GuiElementTypeEnum::InputButtonDecrement, m == ElementModeEnum::Focus ? ElementModeEnum::Default : m, leftPos, ss);
					m = mode(rightPos, ss);
					emitElement(GuiElementTypeEnum::InputButtonIncrement, m == ElementModeEnum::Focus ? ElementModeEnum::Default : m, rightPos, ss);
				}
				hierarchy->text->emit(textPos, textSize);
			}

			bool insideButton(Vec2 pos, Vec2 point)
			{
				return pointInside(pos, Vec2(skin->defaults.inputBox.buttonsWidth, mainSize[1]), point);
			}

			void increment(int sign)
			{
				if (data.value.empty())
				{
					data.value = "0";
					consolidate(); // apply min/max
				}
				try
				{
					detail::OverrideBreakpoint ob;
					if (data.type == InputTypeEnum::Real)
					{
						Real v = toFloat(data.value);
						v += data.step.f * sign;
						data.value = Stringizer() + v.value;
					}
					if (data.type == InputTypeEnum::Integer)
					{
						sint32 v = toSint32(data.value);
						v += data.step.i * sign;
						data.value = Stringizer() + v;
					}
					consolidate(); // apply min/max
				}
				catch (...)
				{
					// do nothing
				}
				hierarchy->fireWidgetEvent();
			}

			void gainFocus()
			{
				// update cursor
				uint32 &cur = data.cursor;
				const uint32 len = utf32Length(data.value);
				cur = min(cur, len);
				if (any(data.style & InputStyleFlags::GoToEndOnFocusGain))
					cur = len;
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				if (!hasFocus())
					gainFocus();
				makeFocused();
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				
				// numeric buttons
				if (data.type == InputTypeEnum::Real || data.type == InputTypeEnum::Integer)
				{
					if (insideButton(leftPos, point))
					{
						increment(-1);
						return true;
					}
					if (insideButton(rightPos, point))
					{
						increment(1);
						return true;
					}
				}

				// update cursor
				hierarchy->text->updateCursorPosition(textPos, textSize, point, data.cursor);

				return true;
			}

			bool keyRepeat(uint32 key, ModifiersFlags modifiers) override
			{
				uint32 &cursor = data.cursor;
				const uint32 len = utf32Length(data.value);
				std::vector<uint32> utf32;
				utf32.resize(len);
				PointerRange<uint32> utf32pr = utf32;
				utf8to32(utf32pr, data.value);
				CAGE_ASSERT(utf32pr.size() == utf32.size());
				CAGE_ASSERT(cursor <= len);
				switch (key)
				{
				case 263: // left
				{
					if (cursor > 0)
						cursor--;
				} break;
				case 262: // right
				{
					if (cursor < len)
						cursor++;
				} break;
				case 259: // backspace
				{
					if (len == 0 || cursor == 0)
						break;
					cursor--;
					utf32.erase(utf32.begin() + cursor);
					data.value = utf32to8string(utf32);
					validate();
					hierarchy->fireWidgetEvent();
				} break;
				case 261: // delete
				{
					if (cursor == len)
						break;
					utf32.erase(utf32.begin() + cursor);
					data.value = utf32to8string(utf32);
					validate();
					hierarchy->fireWidgetEvent();
				} break;
				}
				return true;
			}

			bool keyChar(uint32 key) override
			{
				if (data.value.length() + 6 >= String::MaxLength)
					return true;
				uint32 &cursor = data.cursor;
				const uint32 len = utf32Length(data.value);
				std::vector<uint32> utf32;
				utf32.reserve(len + 1);
				utf32.resize(len);
				PointerRange<uint32> utf32pr = utf32;
				utf8to32(utf32pr, data.value);
				CAGE_ASSERT(utf32pr.size() == utf32.size());
				CAGE_ASSERT(cursor <= len);
				utf32.insert(utf32.begin() + cursor, key);
				data.value = utf32to8string(utf32);
				cursor++;
				validate();
				hierarchy->fireWidgetEvent();
				return true;
			}
		};
	}

	void InputCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<InputImpl>(item).cast<BaseItem>();
	}
}
