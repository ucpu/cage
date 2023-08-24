#include <atomic>

#include <cage-core/debug.h>

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
		void makeLogThrow(const std::source_location &location, const String &message) noexcept
		{
			if (SeverityEnum::Note < getExceptionSilenceSeverity())
				return;
			makeLog(location, SeverityEnum::Note, "exception", message, false, false);
		}
	}

	// exception

	Exception::Exception(const std::source_location &location, SeverityEnum severity, StringPointer message) noexcept : location(location), message(message), severity(severity) {}

	Exception::~Exception() noexcept {}

	void Exception::makeLog() const
	{
		if (severity < getExceptionSilenceSeverity())
			return;
		log();
		::cage::detail::debugBreakpoint();
	}

	void Exception::log() const
	{
		::cage::privat::makeLog(location, severity, "exception", +message, false, false);
	}

	// NotImplemented

	void NotImplemented::log() const
	{
		::cage::privat::makeLog(location, severity, "exception", String() + "not implemented: '" + +message + "'", false, false);
	}

	// SystemError

	SystemError::SystemError(const std::source_location &location, SeverityEnum severity, StringPointer message, sint64 code) noexcept : Exception(location, severity, message), code(code) {}

	void SystemError::log() const
	{
		::cage::privat::makeLog(location, SeverityEnum::Note, "exception", Stringizer() + "code: " + code, false, false);
		::cage::privat::makeLog(location, severity, "exception", +message, false, false);
	}
}
