#include <cage-core/textPack.h>
#include <cage-core/string.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>

#include <unordered_dense.h>

namespace cage
{
	namespace
	{
		class TextPackImpl : public TextPack
		{
		public:
			ankerl::unordered_dense::map<uint32, String> texts;
		};
	}

	String TextPack::format(const String &format, PointerRange<const String> params)
	{
		String res = format;
		while (true)
		{
			String prev = split(res, "{");
			if (res == "")
				return prev + res;
			String mid = split(res, "}");
			if (isDigitsOnly(mid))
			{
				uint32 idx = toUint32(mid);
				if (idx < params.size())
					mid = params[idx];
				else
					mid = "";
			}
			else
				mid = "";
			res = prev + mid + res;
		}
	}

	void TextPack::clear()
	{
		TextPackImpl *impl = (TextPackImpl*)this;
		impl->texts.clear();
	}

	Holder<TextPack> TextPack::copy() const
	{
		Holder<TextPack> res = newTextPack();
		res->importBuffer(exportBuffer()); // todo more efficient
		return res;
	}

	Holder<PointerRange<char>> TextPack::exportBuffer() const
	{
		TextPackImpl *impl = (TextPackImpl *)this;
		MemoryBuffer buff;
		Serializer ser(buff);
		ser << numeric_cast<uint32>(impl->texts.size());
		for (const auto &it : impl->texts)
			ser << it.first << it.second;
		return PointerRangeHolder<char>(PointerRange<char>(buff));
	}

	void TextPack::importBuffer(PointerRange<const char> buffer)
	{
		TextPackImpl *impl = (TextPackImpl *)this;
		clear();
		Deserializer des(buffer);
		uint32 cnt = 0;
		des >> cnt;
		impl->texts.reserve(cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			uint32 n = 0;
			String s;
			des >> n >> s;
			set(n, s);
		}
		CAGE_ASSERT(des.available() == 0);
	}

	void TextPack::set(uint32 name, const String &text)
	{
		CAGE_ASSERT(name != 0);
		TextPackImpl *impl = (TextPackImpl *)this;
		impl->texts[name] = text;
	}

	void TextPack::erase(uint32 name)
	{
		CAGE_ASSERT(name != 0);
		TextPackImpl *impl = (TextPackImpl *)this;
		impl->texts.erase(name);
	}

	String TextPack::get(uint32 name) const
	{
		CAGE_ASSERT(name != 0);
		const TextPackImpl *impl = (const TextPackImpl *)this;
		auto it = impl->texts.find(name);
		if (it == impl->texts.end())
			return "";
		return it->second;
	}

	String TextPack::format(uint32 name, PointerRange<const String> params) const
	{
		CAGE_ASSERT(name != 0);
		return format(get(name), params);
	}

	Holder<TextPack> newTextPack()
	{
		return systemMemory().createImpl<TextPack, TextPackImpl>();
	}
}
