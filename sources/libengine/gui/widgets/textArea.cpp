#include "../private.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/utf.h>

namespace cage
{
	void textCreate(HierarchyItem *item);

	namespace
	{
		struct TextAreaImpl : public WidgetItem
		{
			GuiTextAreaComponent &data;
			MemoryBuffer &buffer;
			Vec2 textPos, textSize;

			TextAreaImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(TextArea)), buffer(*data.buffer) { CAGE_ASSERT(data.buffer); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);

				if (!hierarchy->text)
					textCreate(hierarchy);
				hierarchy->text->skipInitialize = true;
				hierarchy->text->apply(skin->defaults.textArea.textFormat);
				if (GUI_HAS_COMPONENT(TextFormat, hierarchy->ent))
				{
					GUI_COMPONENT(TextFormat, f, hierarchy->ent);
					hierarchy->text->apply(f);
				}
				hierarchy->text->transcript(buffer);

				if (hasFocus())
				{
					data.cursor = min(data.cursor, utf32Length(buffer));
					hierarchy->text->cursor = data.cursor;
				}
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.textArea.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.textArea.margin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				textPos = hierarchy->renderPos;
				textSize = hierarchy->renderSize;
				offset(textPos, textSize, -skin->defaults.textArea.margin - skin->layouts[(uint32)GuiElementTypeEnum::TextArea].border - skin->defaults.textArea.padding);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.textArea.margin);
				emitElement(GuiElementTypeEnum::TextArea, mode(), p, s);
				hierarchy->text->emit(textPos, textSize);
			}

			void gainFocus()
			{
				// update cursor
				uint32 &cur = data.cursor;
				const uint32 len = utf32Length(buffer);
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

				// update cursor
				hierarchy->text->updateCursorPosition(textPos, textSize, point, data.cursor);

				return true;
			}

			void store(const std::vector<uint32> &utf32)
			{
				buffer.resizeSmart(utf8Length(utf32));
				PointerRange<char> utf8pr = buffer;
				utf32to8(utf8pr, utf32);
				CAGE_ASSERT(utf8pr.size() == buffer.size());
				hierarchy->fireWidgetEvent();
			}

			bool keyRepeat(uint32 key, ModifiersFlags modifiers) override
			{
				uint32 &cursor = data.cursor;
				const uint32 len = utf32Length(buffer);
				std::vector<uint32> utf32;
				utf32.resize(len);
				PointerRange<uint32> utf32pr = utf32;
				utf8to32(utf32pr, buffer);
				CAGE_ASSERT(utf32pr.size() == utf32.size());
				CAGE_ASSERT(cursor <= len);
				switch (key)
				{
					case 263: // left
					{
						if (cursor > 0)
							cursor--;
					}
					break;
					case 262: // right
					{
						if (cursor < len)
							cursor++;
					}
					break;
					case 259: // backspace
					{
						if (len == 0 || cursor == 0)
							break;
						cursor--;
						utf32.erase(utf32.begin() + cursor);
						store(utf32);
					}
					break;
					case 261: // delete
					{
						if (cursor == len)
							break;
						utf32.erase(utf32.begin() + cursor);
						store(utf32);
					}
					break;
				}
				return true;
			}

			bool keyChar(uint32 key) override
			{
				if (buffer.size() + 6 >= data.maxLength)
					return true;
				uint32 &cursor = data.cursor;
				const uint32 len = utf32Length(buffer);
				std::vector<uint32> utf32;
				utf32.reserve(len + 1);
				utf32.resize(len);
				PointerRange<uint32> utf32pr = utf32;
				utf8to32(utf32pr, buffer);
				CAGE_ASSERT(utf32pr.size() == utf32.size());
				CAGE_ASSERT(cursor <= len);
				utf32.insert(utf32.begin() + cursor, key);
				cursor++;
				store(utf32);
				return true;
			}
		};
	}

	void TextAreaCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<TextAreaImpl>(item).cast<BaseItem>();
	}
}
