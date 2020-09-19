#include <cage-core/math.h>
#include <cage-core/string.h>

namespace cage
{
	namespace privat
	{
		inline string tryRemoveParentheses(const string &str)
		{
			if (str.empty())
				return str;
			if (str[0] == '(' && str[str.length() - 1] == ')')
				return subString(str, 1, str.length() - 2);
			return str;
		}

		inline string mathSplit(string &s)
		{
			string res = trim(split(s, "\t ,"), true, true, "\t ");
			s = trim(s, true, true, "\t ");
			return res;
		}
	}
}
