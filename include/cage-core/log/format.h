namespace cage
{
	CAGE_API string severityToString(const severityEnum severity);

	CAGE_API void logFormatPolicyConsole(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyFileShort(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyFileLong(const detail::loggerInfo &info, delegate<void(const string &)> output);
}
