#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utf.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	void textCreate(hierarchyItemStruct *item);

	namespace
	{
		struct inputImpl : public widgetItemStruct
		{
			inputComponent &data;
			selectionComponent &selection;
			bool showArrows;

			vec2 leftPos, rightPos;
			vec2 mainPos, mainSize;
			vec2 textPos, textSize;

			inputImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(input)), selection(GUI_REF_COMPONENT(selection)), showArrows(false)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!hierarchy->firstChild, "input box may not have children");
				CAGE_ASSERT_RUNTIME(!hierarchy->image, "input box may not have image");

				showArrows = data.type == inputTypeEnum::Real || data.type == inputTypeEnum::Integer;

				switch (data.type)
				{
				case inputTypeEnum::Integer:
					CAGE_ASSERT_RUNTIME(data.step.i >= 0);
					CAGE_ASSERT_RUNTIME(data.max.i >= data.min.i);
					break;
				case inputTypeEnum::Real:
					CAGE_ASSERT_RUNTIME(data.min.f.valid() && data.step.f.valid() && data.max.f.valid());
					CAGE_ASSERT_RUNTIME(data.step.f >= 0);
					CAGE_ASSERT_RUNTIME(data.max.f >= data.min.f);
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
					if (data.type == inputTypeEnum::Password)
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
				if ((data.style & inputStyleFlags::AlwaysRoundValueToStep) == inputStyleFlags::AlwaysRoundValueToStep && (value % s) != 0)
					value -= value % s;
				return clamp(value, a, b);
			}

			void consolidate()
			{
				try
				{
					detail::overrideBreakpoint ob;
					switch (data.type)
					{
					case inputTypeEnum::Integer:
						if (data.value.isInteger(true))
							data.value = consolidate<sint32>(data.value.toSint32(), data.min.i, data.max.i, data.step.i);
						break;
					case inputTypeEnum::Real:
						if (data.value.isReal(true))
							data.value = consolidate<real>(data.value.toFloat(), data.min.f, data.max.f, data.step.f);
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
					detail::overrideBreakpoint ob;
					switch (data.type)
					{
					case inputTypeEnum::Text:
						data.valid = true;
						break;
					case inputTypeEnum::Integer:
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
					case inputTypeEnum::Real:
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
					case inputTypeEnum::Email:
						// todo
						break;
					case inputTypeEnum::Url:
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

			virtual void findFinalPosition(const finalPositionStruct &update) override
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
					case inputButtonsPlacementModeEnum::Left:
						rightPos[0] += bwo;
						mainPos[0] += bwo * 2;
						break;
					case inputButtonsPlacementModeEnum::Right:
						leftPos[0] += mw + off;
						rightPos[0] += mw + bwo + off;
						break;
					case inputButtonsPlacementModeEnum::Sides:
						mainPos[0] += bwo;
						rightPos[0] += bwo + mw + off;
						break;
					default:
						CAGE_THROW_CRITICAL(exception, "invalid input buttons placement enum");
					}
				}
				textPos = mainPos;
				textSize = mainSize;
				offset(textPos, textSize, -skin->layouts[(uint32)elementTypeEnum::Input].border - s.basePadding);
			}

			virtual void emit() const override
			{
				const auto &s = skin->defaults.inputBox;
				emitElement(elementTypeEnum::Input, mode(mainPos, mainSize), mainPos, mainSize);
				if (showArrows)
				{
					vec2 ss = vec2(s.buttonsSize, mainSize[1]);
					uint32 m = mode(leftPos, ss);
					emitElement(elementTypeEnum::InputButtonDecrement, m == 1 ? 0 : m, leftPos, ss);
					m = mode(rightPos, ss);
					emitElement(elementTypeEnum::InputButtonIncrement, m == 1 ? 0 : m, rightPos, ss);
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
					detail::overrideBreakpoint ob;
					if (data.type == inputTypeEnum::Real)
					{
						real v = data.value.toFloat();
						v += data.step.f * sign;
						data.value = v;
					}
					if (data.type == inputTypeEnum::Integer)
					{
						sint32 v = data.value.toSint32();
						v += data.step.i * sign;
						data.value = v;
					}
					consolidate(); // apply min/max
				}
				catch (...)
				{
					// do nothing
				}
				hierarchy->impl->widgetEvent.dispatch(hierarchy->ent->name());
			}

			void gainFocus()
			{
				// update cursor
				uint32 len = countCharacters(data.value);
				uint32 &cur = data.cursor;
				cur = min(cur, len);
				if ((data.style & inputStyleFlags::GoToEndOnFocusGain) == inputStyleFlags::GoToEndOnFocusGain)
					cur = len;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (!hasFocus())
					gainFocus();
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				
				// numeric buttons
				if (data.type == inputTypeEnum::Real || data.type == inputTypeEnum::Integer)
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

			virtual bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers) override
			{
				uint32 &cursor = data.cursor;
				std::vector<uint32> utf32;
				utf32.resize(countCharacters(data.value));
				uint32 len;
				convert8to32(utf32.data(), len, data.value);
				CAGE_ASSERT_RUNTIME(len == utf32.size(), len, utf32.size());
				CAGE_ASSERT_RUNTIME(cursor <= len, cursor, len, data.value);
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
					hierarchy->impl->widgetEvent.dispatch(hierarchy->ent->name());
				} break;
				case 261: // delete
				{
					if (cursor == len)
						break;
					utf32.erase(utf32.begin() + cursor);
					data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
					validate();
					hierarchy->impl->widgetEvent.dispatch(hierarchy->ent->name());
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
				CAGE_ASSERT_RUNTIME(len == utf32.size(), len, utf32.size());
				CAGE_ASSERT_RUNTIME(cursor <= len, cursor, len, data.value);
				utf32.insert(utf32.begin() + cursor, key);
				data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
				cursor++;
				validate();
				hierarchy->impl->widgetEvent.dispatch(hierarchy->ent->name());
				return true;
			}
		};
	}

	void inputCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<inputImpl>(item);
	}
}
