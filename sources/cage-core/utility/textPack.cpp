#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/textPack.h>

namespace cage
{
	string textPackClass::format(const string &format, uint32 paramCount, const string *paramValues)
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
			//if (prev.length() + mid.length() + res.length() >= string::MaxLength)
			//	CAGE_THROW_ERROR(exception, "string too long");
			res = prev + mid + res;
		}
	}

	namespace
	{
		class textPackImpl : public textPackClass
		{
		public:
			std::map<uint32, string> texts;
		};
	}

	void textPackClass::set(uint32 name, const string &text)
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts[name] = text;
	}

	void textPackClass::erase(uint32 name)
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts.erase(name);
	}

	void textPackClass::clear()
	{
		textPackImpl *impl = (textPackImpl*)this;
		impl->texts.clear();
	}

	string textPackClass::get(uint32 name) const
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		textPackImpl *impl = (textPackImpl*)this;
		auto it = impl->texts.find(name);
		if (it == impl->texts.end())
			return "";
		return it->second;
	}

	string textPackClass::format(uint32 name, uint32 paramCount, const string *paramValues) const
	{
		CAGE_ASSERT_RUNTIME(name != 0);
		return format(get(name), paramCount, paramValues);
	}

	holder<textPackClass> newTextPack()
	{
		return detail::systemArena().createImpl<textPackClass, textPackImpl>();
	}
}