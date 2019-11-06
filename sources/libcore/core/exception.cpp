#define CAGE_EXPORT
#include <cage-core/core.h>

#include <atomic>

namespace cage
{
	// overrides

	namespace
	{
		severityEnum &localExceptionSilenceSeverity()
		{
			thread_local severityEnum sev = (severityEnum)0;
			return sev;
		}

		std::atomic<severityEnum> &globalExceptionSilenceSeverity()
		{
			static std::atomic<severityEnum> sev = { (severityEnum)0 };
			return sev;
		}
	}

	namespace detail
	{
		overrideException::overrideException(severityEnum severity) : original(localExceptionSilenceSeverity())
		{
			localExceptionSilenceSeverity() = severity;
		}

		overrideException::~overrideException()
		{
			localExceptionSilenceSeverity() = original;
		}

		void setGlobalExceptionOverride(severityEnum severity)
		{
			globalExceptionSilenceSeverity() = severity;
		}
	}

	namespace
	{
		severityEnum getExceptionSilenceSeverity()
		{
			return localExceptionSilenceSeverity() > (severityEnum)globalExceptionSilenceSeverity() ? localExceptionSilenceSeverity() : (severityEnum)globalExceptionSilenceSeverity();
		}
	}

	// exception

	exception::exception(const char *file, uint32 line, const char *function, severityEnum severity, const char *message) noexcept : file(file), function(function), line(line), message(message), severity(severity)
	{};

	exception::~exception() noexcept
	{};

	void exception::makeLog()
	{
		if (severity < getExceptionSilenceSeverity())
			return;
		log();
		::cage::detail::debugBreakpoint();
	}

	void exception::log()
	{
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};

	// notImplemented

	notImplemented::notImplemented(const char *file, uint32 line, const char *function, severityEnum severity, const char *message) noexcept : exception(file, line, function, severity, message)
	{};

	void notImplemented::log()
	{
		::cage::privat::makeLog(file, line, function, severity, "exception", string() + "not implemented: '" + message + "'", false, false);
	};

	// systemError

	systemError::systemError(const char *file, uint32 line, const char *function, severityEnum severity, const char *message, uint32 code) noexcept : exception(file, line, function, severity, message), code(code)
	{};

	void systemError::log()
	{
		::cage::privat::makeLog(file, line, function, severityEnum::Note, "exception", stringizer() + "code: " + code, false, false);
		::cage::privat::makeLog(file, line, function, severity, "exception", message, false, false);
	};
}
