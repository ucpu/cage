namespace cage
{
	namespace privat
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
			string res = s.split("\t ,").trim(true, true, "\t ");
			s = s.trim(true, true, "\t ");
			return res;
		}
	}
}
