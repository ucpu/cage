#include <cage-core/debug.h>

#include <atomic>

namespace cage
{
	// overrides

	namespace
	{
		SeverityEnum &localExceptionSilenceSeverity()
		{
			thread_local SeverityEnum sev = (SeverityEnum)0;
			return sev;
		}

		std::atomic<SeverityEnum> &globalExceptionSilenceSeverity()
		{
			static std::atomic<SeverityEnum> sev = { (SeverityEnum)0 };
			return sev;
		}
	}

	namespace detail
	{
		OverrideException::OverrideException(SeverityEnum severity) : original(localExceptionSilenceSeverity())
		{
			localExceptionSilenceSeverity() = severity;
		}

		OverrideException::~OverrideException()
		{
			localExceptionSilenceSeverity() = original;
		}

		void setGlobalExceptionOverride(SeverityEnum severity)
		{
			globalExceptionSilenceSeverity() = severity;
		}
	}

	namespace
	{
		SeverityEnum getExceptionSilenceSeverity()
		{
			return localExceptionSilenceSeverity() > (SeverityEnum)globalExceptionSilenceSeverity() ? localExceptionSilenceSeverity() : (SeverityEnum)globalExceptionSilenceSeverity();
		}
	}

	namespace privat
	{
		void makeLogThrow(const char *file, uint32 line, const char *function, const string &message) noexcept
		{
			if (SeverityEnum::Note < getExceptionSilenceSeverity())
				return;
			makeLog(file, line, function, SeverityEnum::Note, "exception", message, false, false);
		}
	}

	// exception

	Exception::Exception(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept : file(file), function(function), line(line), message(message), severity(severity)
	{};

	Exception::~Exception() noexcept
	{};

	void Exception::makeLog()
	{
		if (severity < getExceptionSilenceSeverity())
			return;
		log();
		::cage::detail::debugBreakpoint();
	}

	void Exception::log()
	{
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};

	// NotImplemented

	void NotImplemented::log()
	{
		::cage::privat::makeLog(file, line, function, severity, "exception", string() + "not implemented: '" + message + "'", false, false);
	};

	// SystemError

	SystemError::SystemError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, sint64 code) noexcept : Exception(file, line, function, severity, message), code(code)
	{};

	void SystemError::log()
	{
		::cage::privat::makeLog(file, line, function, SeverityEnum::Note, "exception", stringizer() + "code: " + code, false, false);
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};
}
