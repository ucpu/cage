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
			static std::atomic<SeverityEnum> sev = (SeverityEnum)0;
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

		void globalExceptionOverride(SeverityEnum severity)
		{
			globalExceptionSilenceSeverity() = severity;
		}
	}

	namespace
	{
		SeverityEnum getExceptionSilenceSeverity()
		{
			SeverityEnum l = localExceptionSilenceSeverity();
			SeverityEnum g = (SeverityEnum)globalExceptionSilenceSeverity();
			return l > g ? l : g;
		}
	}

	namespace privat
	{
		void makeLogThrow(StringLiteral function, StringLiteral file, uint32 line, const String &message) noexcept
		{
			if (SeverityEnum::Note < getExceptionSilenceSeverity())
				return;
			makeLog(function, file, line, SeverityEnum::Note, "exception", message, false, false);
		}
	}

	// exception

	Exception::Exception(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral message) noexcept : function(function), file(file), line(line), severity(severity), message(message)
	{}

	Exception::~Exception() noexcept
	{}

	void Exception::makeLog() const
	{
		if (severity < getExceptionSilenceSeverity())
			return;
		log();
		::cage::detail::debugBreakpoint();
	}

	void Exception::log() const
	{
		::cage::privat::makeLog(function, file, line, severity, "exception", +message, false, false);
	}

	// NotImplemented

	void NotImplemented::log() const
	{
		::cage::privat::makeLog(function, file, line, severity, "exception", String() + "not implemented: '" + +message + "'", false, false);
	}

	// SystemError

	SystemError::SystemError(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral message, sint64 code) noexcept : Exception(function, file, line, severity, message), code(code)
	{}

	void SystemError::log() const
	{
		::cage::privat::makeLog(function, file, line, SeverityEnum::Note, "exception", Stringizer() + "code: " + code, false, false);
		::cage::privat::makeLog(function, file, line, severity, "exception", +message, false, false);
	}
}
