#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/textPack.h>

namespace cage
{
	string textPack::format(const string &format, uint32 paramCount, const string *paramValues)
	{
		string res = format;
		while (true)
		{
			string prev = res.split("{");
			if (res == "")
				return prev + res;
			string mid = res.split("}");
			if (mid.isDigitsOnly())
			{
				uint32 idx = mid.toUint32();
				if (idx < paramCount)
					mid = paramValues[idx];
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
		class textPackImpl : public textPack
		{
		public:
			std::map<uint32, string> texts;
		};
	}

	void textPack::set(uint32 name, const string &text)
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts[name] = text;
	}

	void textPack::erase(uint32 name)
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts.erase(name);
	}

	void textPack::clear()
	{
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts.clear();
	}

	string textPack::get(uint32 name) const
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		auto it = impl->texts.find(name);
		if (it == impl->texts.end())
			return "";
		return it->second;
	}

	string textPack::format(uint32 name, uint32 paramCount, const string *paramValues) const
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		return format(get(name), paramCount, paramValues);
	}

	holder<textPack> newTextPack()
	{
		return detail::systemArena().createImpl<textPack, textPackImpl>();
	}
}
