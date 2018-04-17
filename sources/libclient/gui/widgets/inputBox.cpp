#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/utf.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	void textCreate(guiItemStruct *item);

	namespace
	{
		struct inputBoxImpl : public widgetBaseStruct
		{
			inputBoxComponent &data;
			selectionComponent &selection;

			vec2 leftPos, rightPos;
			vec2 mainPos, mainSize;

			inputBoxImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(inputBox)), selection(GUI_REF_COMPONENT(selection))
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "input box may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "input box may not have layout");
				CAGE_ASSERT_RUNTIME(!base->image, "input box may not have image");

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
				}

				if (hasFocus())
					validate();
				else
					consolidate();

				if (!base->text)
					textCreate(base);
				base->text->skipInitialize = true;
				if (data.value.empty() && !hasFocus())
				{ // placeholder
					base->text->text.apply(skin().defaults.inputBox.placeholderFormat, base->impl);
					base->text->transcript();
				}
				else
				{ // actual value
					if (data.valid)
						base->text->text.apply(skin().defaults.inputBox.textValidFormat, base->impl);
					else
						base->text->text.apply(skin().defaults.inputBox.textInvalidFormat, base->impl);
					if (data.type == inputTypeEnum::Password)
					{
						base->text->transcript("*");
						uint32 g = base->text->text.glyphs[0];
						base->text->transcript(data.value);
						for (uint32 i = 0; i < base->text->text.count; i++)
							base->text->text.glyphs[i] = g;
					}
					else
						base->text->transcript(data.value);
				}

				if (hasFocus())
				{
					data.cursor = min(data.cursor, countCharacters(data.value));
					base->text->text.cursor = data.cursor;
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

			virtual void updateRequestedSize() override
			{
				base->requestedSize = skin().defaults.inputBox.size;
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				const auto &s = skin().defaults.inputBox;
				base->updateContentPosition(s.margin);
				leftPos = mainPos = rightPos = base->contentPosition;
				mainSize = base->contentSize;
				if (data.type == inputTypeEnum::Real || data.type == inputTypeEnum::Integer)
					mainSize[0] -= (s.buttonsSize + s.buttonsOffset) * 2;
				if (data.type == inputTypeEnum::Real || data.type == inputTypeEnum::Integer)
				{
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
				base->contentPosition = mainPos;
				base->contentSize = mainSize;
				offset(base->contentPosition, base->contentSize, -skin().layouts[(uint32)elementTypeEnum::InputBox].border - s.basePadding);
			}

			virtual void emit() const override
			{
				const auto &s = skin().defaults.inputBox;
				emitElement(elementTypeEnum::InputBox, mode(mainPos, mainSize), mainPos, mainSize);
				if (data.type == inputTypeEnum::Real || data.type == inputTypeEnum::Integer)
				{
					vec2 ss = vec2(s.buttonsSize, mainSize[1]);
					uint32 m = mode(leftPos, ss);
					emitElement(elementTypeEnum::InputButtonDecrement, m == 1 ? 0 : m, leftPos, ss);
					m = mode(rightPos, ss);
					emitElement(elementTypeEnum::InputButtonIncrement, m == 1 ? 0 : m, rightPos, ss);
				}
				base->text->emit();
			}

			bool insideButton(vec2 pos, vec2 point)
			{
				return pointInside(pos, vec2(skin().defaults.inputBox.buttonsSize, mainSize[1]), point);
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
				base->impl->widgetEvent.dispatch(base->entity->getName());
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
				base->text->updateCursorPosition(base->contentPosition, base->contentSize, point, data.cursor);

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
					base->impl->widgetEvent.dispatch(base->entity->getName());
				} break;
				case 261: // delete
				{
					if (cursor == len)
						break;
					utf32.erase(utf32.begin() + cursor);
					data.value = convert32to8(utf32.data(), numeric_cast<uint32>(utf32.size()));
					validate();
					base->impl->widgetEvent.dispatch(base->entity->getName());
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
				base->impl->widgetEvent.dispatch(base->entity->getName());
				return true;
			}
		};
	}

	void inputBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<inputBoxImpl>(item);
	}
}
