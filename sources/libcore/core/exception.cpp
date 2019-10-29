#define CAGE_EXPORT
#include <cage-core/core.h>

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

	void exception::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return;
		GCHL_EXCEPTION_GENERATE_LOG(message);
	};

	// notImplemented

	notImplemented::notImplemented(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	void notImplemented::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return;
		GCHL_EXCEPTION_GENERATE_LOG(string() + "not implemented: '" + message + "'");
	};

	// outOfMemory

	outOfMemory::outOfMemory(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uintPtr memory) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER), memory(memory)
	{};

	void outOfMemory::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return;
		CAGE_LOG_CONTINUE(severityEnum::Note, "exception", string("memory requested: ") + memory);
		GCHL_EXCEPTION_GENERATE_LOG(message);
	};

	// codeException

	codeException::codeException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER), code(code)
	{};

	void codeException::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return;
		CAGE_LOG_CONTINUE(severityEnum::Note, "exception", string("code: ") + code);
		GCHL_EXCEPTION_GENERATE_LOG(message);
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
