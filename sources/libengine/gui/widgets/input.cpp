#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utf.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
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
			bool showArrows;

			vec2 leftPos, rightPos;
			vec2 mainPos, mainSize;
			vec2 textPos, textSize;

			InputImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Input)), selection(GUI_REF_COMPONENT(Selection)), showArrows(false)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "input box may not have children");
				CAGE_ASSERT(!hierarchy->Image, "input box may not have image");

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
					hierarchy->text->text.apply(skin->defaults.inputBox.placeholderFormat, hierarchy->impl);
					hierarchy->text->transcript();
				}
				else
				{ // actual value
					if (data.valid)
						hierarchy->text->text.apply(skin->defaults.inputBox.textValidFormat, hierarchy->impl);
					else
						hierarchy->text->text.apply(skin->defaults.inputBox.textInvalidFormat, hierarchy->impl);
					if (data.type == InputTypeEnum::Password)
					{
						hierarchy->text->transcript("*");
						uint32 g = hierarchy->text->text.glyphs[0];
						hierarchy->text->transcript(data.value);
						for (uint32 i = 0; i < hierarchy->text->text.count; i++)
							hierarchy->text->text.glyphs[i] = g;
					}
					else
						hierarchy->text->transcript(data.value);
				}

				if (hasFocus())
				{
					data.cursor = min(data.cursor, countCharacters(data.value));
					hierarchy->text->text.cursor = data.cursor;
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
						if (data.value.isInteger(true))
							data.value = string(consolidate<sint32>(data.value.toSint32(), data.min.i, data.max.i, data.step.i));
						break;
					case InputTypeEnum::Real:
						if (data.value.isReal(true))
							data.value = string(consolidate<real>(data.value.toFloat(), data.min.f, data.max.f, data.step.f).value);
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
						if (data.value.isInteger(true))
						{
							sint32 v = data.value.toSint32();
							sint32 r = consolidate<sint32>(v, data.min.i, data.max.i, data.step.i);
							data.valid = v == r;
						}
						else
							data.valid = false;
					} break;
					case InputTypeEnum::Real:
					{
						if (data.value.isReal(true))
						{
							real v = data.value.toFloat();
							real r = consolidate<real>(v, data.min.f, data.max.f, data.step.f);
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

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.inputBox.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.inputBox.margin);
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				const auto &s = skin->defaults.inputBox;
				mainPos = hierarchy->renderPos;
				mainSize = hierarchy->renderSize;
				offset(mainPos, mainSize, -s.margin);
				leftPos = rightPos = mainPos;
				if (showArrows)
				{
					mainSize[0] -= (s.buttonsSize + s.buttonsOffset) * 2;
					real mw = mainSize[0];
					real bw = s.buttonsSize;
					real off = s.buttonsOffset;
					real bwo = bw + off;
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

			virtual void emit() const override
			{
				const auto &s = skin->defaults.inputBox;
				emitElement(GuiElementTypeEnum::Input, mode(mainPos, mainSize), mainPos, mainSize);
				if (showArrows)
				{
					vec2 ss = vec2(s.buttonsSize, mainSize[1]);
					uint32 m = mode(leftPos, ss);
					emitElement(GuiElementTypeEnum::InputButtonDecrement, m == 1 ? 0 : m, leftPos, ss);
					m = mode(rightPos, ss);
					emitElement(GuiElementTypeEnum::InputButtonIncrement, m == 1 ? 0 : m, rightPos, ss);
				}
				hierarchy->text->emit(textPos, textSize);
			}

			bool insideButton(vec2 pos, vec2 point)
			{
				return pointInside(pos, vec2(skin->defaults.inputBox.buttonsSize, mainSize[1]), point);
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
						real v = data.value.toFloat();
						v += data.step.f * sign;
						data.value = string(v.value);
					}
					if (data.type == InputTypeEnum::Integer)
					{
						sint32 v = data.value.toSint32();
						v += data.step.i * sign;
						data.value = string(v);
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
				uint32 len = countCharacters(data.value);
				uint32 &cur = data.cursor;
				cur = min(cur, len);
				if ((data.style & InputStyleFlags::GoToEndOnFocusGain) == InputStyleFlags::GoToEndOnFocusGain)
					cur = len;
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
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

			virtual bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers) override
			{
				uint32 &cursor = data.cursor;
				std::vector<uint32> utf32;
				utf32.resize(countCharacters(data.value));
				uint32 len;
				convert8to32(utf32.data(), len, data.value);
				CAGE_ASSERT(len == utf32.size(), len, utf32.size());
				CAGE_ASSERT(cursor <= len, cursor, len, data.value);
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
					data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
					validate();
					hierarchy->fireWidgetEvent();
				} break;
				case 261: // delete
				{
					if (cursor == len)
						break;
					utf32.erase(utf32.begin() + cursor);
					data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
					validate();
					hierarchy->fireWidgetEvent();
				} break;
				}
				return true;
			}

			virtual bool keyChar(uint32 key) override
			{
				if (data.value.length() + 1 >= string::MaxLength)
					return true;
				uint32 &cursor = data.cursor;
				std::vector<uint32> utf32;
				uint32 len = countCharacters(data.value);
				utf32.reserve(len + 1);
				utf32.resize(len);
				convert8to32(utf32.data(), len, data.value);
				CAGE_ASSERT(len == utf32.size(), len, utf32.size());
				CAGE_ASSERT(cursor <= len, cursor, len, data.value);
				utf32.insert(utf32.begin() + cursor, key);
				data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
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
		item->item = item->impl->itemsMemory.createObject<InputImpl>(item);
	}
}
