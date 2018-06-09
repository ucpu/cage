namespace cage
{
	namespace detail
	{
		inline string tryRemoveParentheses(const string &str)
		{
			if (str.empty())
				return str;
			if (str[0] == '(' && str[str.length() - 1] == ')')
				return str.subString(1, str.length() - 2);
			return str;
		}

		inline string mathSplit(string &s)
		{
			static const string seps = string("\t, ").order();
			static const string trims = string("\t ").order();
			string res = s.split(seps).trim(true, true, trims);
			s = s.trim(true, true, trims);
			return res;
		}
	}
}
