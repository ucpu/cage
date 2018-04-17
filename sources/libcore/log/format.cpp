#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>

namespace cage
{
	string severityToString(severityEnum severity)
	{
		switch (severity)
		{
		case severityEnum::Hint: return "hint";
		case severityEnum::Note: return "note";
		case severityEnum::Info: return "info";
		case severityEnum::Warning: return "warn";
		case severityEnum::Error: return "EROR";
		case severityEnum::Critical: return "CRIT";
		default: return "UNKN";
		}
	}

	void logFormatPolicyConsole(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		output(info.message);
	}

	namespace
	{
		void logFormatPolicyFileImpl(const detail::loggerInfo &info, delegate<void(const string &)> output, bool longer)
		{
			if (info.continuous)
			{
				output(string("\t") + info.message);
			}
			else
			{
				string res;
				res += string(info.time).fill(12) + " ";
				res += string(info.currentThread).fill(10) + " ";
				res += severityToString(info.severity) + " ";
				res += string(info.component).fill(20) + " ";
				res += info.message;
				if (longer && info.file)
				{
					string flf = string(" ") + string(info.file) + ":" + string(info.line) + " (" + string(info.function) + ")";
					if (res.length() + flf.length() + 10 < string::MaxLength)
					{
						res += string().fill(string::MaxLength - flf.length() - res.length() - 5);
						res += flf;
					}
				}
				output(res);
			}
		}
	}

	void logFormatPolicyFileShort(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		logFormatPolicyFileImpl(info, output, false);
	}

	void logFormatPolicyFileLong(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		logFormatPolicyFileImpl(info, output, true);
	}
}
