#include <cage-core/textPack.h>
#include <cage-core/string.h>

#include <map>

namespace cage
{
	string TextPack::format(const string &format, PointerRange<const string> params)
	{
		string res = format;
		while (true)
		{
			string prev = split(res, "{");
			if (res == "")
				return prev + res;
			string mid = split(res, "}");
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

	namespace
	{
		class TextPackImpl : public TextPack
		{
		public:
			std::map<uint32, string> texts;
		};
	}

	void TextPack::set(uint32 name, const string &text)
	{
		CAGE_ASSERT(name != 0);
		TextPackImpl *impl = (TextPackImpl*)this;
		impl->texts[name] = text;
	}

	void TextPack::erase(uint32 name)
	{
		CAGE_ASSERT(name != 0);
		TextPackImpl *impl = (TextPackImpl*)this;
		impl->texts.erase(name);
	}

	void TextPack::clear()
	{
		TextPackImpl *impl = (TextPackImpl*)this;
		impl->texts.clear();
	}

	string TextPack::get(uint32 name) const
	{
		CAGE_ASSERT(name != 0);
		TextPackImpl *impl = (TextPackImpl*)this;
		auto it = impl->texts.find(name);
		if (it == impl->texts.end())
			return "";
		return it->second;
	}

	string TextPack::format(uint32 name, PointerRange<const string> params) const
	{
		CAGE_ASSERT(name != 0);
		return format(get(name), params);
	}

	Holder<TextPack> newTextPack()
	{
		return detail::systemArena().createImpl<TextPack, TextPackImpl>();
	}
}
