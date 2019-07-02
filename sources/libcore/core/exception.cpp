#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>

#include <atomic>

namespace cage
{
	// exception

	exception::exception(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept :
#ifdef CAGE_DEBUG
	file(file), function(function), line(line),
#endif
		message(message), severity(severity)
	{};

	exception::~exception() noexcept
	{};

	exception &exception::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		GCHL_EXCEPTION_GENERATE_LOG(message);
		return *this;
	};

	// notImplemented

	notImplemented::notImplemented(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	notImplemented &notImplemented::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		GCHL_EXCEPTION_GENERATE_LOG(string() + "not implemented: '" + message + "'");
		return *this;
	};

	// outOfMemory

	outOfMemory::outOfMemory(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uintPtr memory) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER), memory(memory)
	{};

	outOfMemory &outOfMemory::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		CAGE_LOG_CONTINUE(severityEnum::Note, "exception", string("memory requested: ") + memory);
		GCHL_EXCEPTION_GENERATE_LOG(message);
		return *this;
	};

	// codeException

	codeException::codeException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER), code(code)
	{};

	codeException &codeException::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		CAGE_LOG_CONTINUE(severityEnum::Note, "exception", string("code: ") + code);
		GCHL_EXCEPTION_GENERATE_LOG(message);
		return *this;
	};

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

		severityEnum getExceptionSilenceSeverity()
		{
			return localExceptionSilenceSeverity() > (severityEnum)globalExceptionSilenceSeverity() ? localExceptionSilenceSeverity() : (severityEnum)globalExceptionSilenceSeverity();
		}
	}
}
