#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>

namespace cage
{
	namespace detail
	{
		namespace
		{
			thread_local severityEnum localExceptionSilenceSeverity = (severityEnum)0;
			severityEnum globalExceptionSilenceSeverity = (severityEnum)0;
		}
	}

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

	// notImplementedException

	notImplementedException::notImplementedException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	notImplementedException &notImplementedException::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		GCHL_EXCEPTION_GENERATE_LOG(string() + "not implemented: '" + message + "'");
		return *this;
	};

	// utf8Exception

	utf8Exception::utf8Exception(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	utf8Exception &utf8Exception::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		GCHL_EXCEPTION_GENERATE_LOG(string("utf8 error: '") + message + "'");
		return *this;
	};

	// disconnectedException

	disconnectedException::disconnectedException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	disconnectedException &disconnectedException::log()
	{
		if (severity < detail::getExceptionSilenceSeverity())
			return *this;
		GCHL_EXCEPTION_GENERATE_LOG(message);
		return *this;
	};

	// outOfMemoryException

	outOfMemoryException::outOfMemoryException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uintPtr memory) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER), memory(memory)
	{};

	outOfMemoryException &outOfMemoryException::log()
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

	namespace detail
	{
		overrideException::overrideException(severityEnum severity) : original(localExceptionSilenceSeverity)
		{
			localExceptionSilenceSeverity = severity;
		}

		overrideException::~overrideException()
		{
			localExceptionSilenceSeverity = original;
		}

		void setGlobalExceptionOverride(severityEnum severity)
		{
			globalExceptionSilenceSeverity = severity;
		}

		severityEnum getExceptionSilenceSeverity()
		{
			return localExceptionSilenceSeverity > globalExceptionSilenceSeverity ? localExceptionSilenceSeverity : globalExceptionSilenceSeverity;
		}
	}
}
