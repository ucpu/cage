namespace cage
{
	CAGE_API string severityToString(const severityEnum severity);

	CAGE_API void logFormatPolicyDebug(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyConsole(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyFile(const detail::loggerInfo &info, delegate<void(const string &)> output);
}
