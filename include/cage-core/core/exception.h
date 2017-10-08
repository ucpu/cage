#ifdef CAGE_DEBUG
#define GCHL_THROW_ARGS __FILE__, __LINE__, __FUNCTION__,
#else
#define GCHL_THROW_ARGS
#endif
#define CAGE_THROW_SILENT(EXCEPTION, ...) { throw EXCEPTION(GCHL_THROW_ARGS ::cage::severityEnum::Error, __VA_ARGS__); }
#define CAGE_THROW_WARNING(EXCEPTION, ...) { throw EXCEPTION(GCHL_THROW_ARGS ::cage::severityEnum::Warning, __VA_ARGS__).log(); }
#define CAGE_THROW_ERROR(EXCEPTION, ...) { throw EXCEPTION(GCHL_THROW_ARGS ::cage::severityEnum::Error, __VA_ARGS__).log(); }
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) { throw EXCEPTION(GCHL_THROW_ARGS ::cage::severityEnum::Critical, __VA_ARGS__).log(); }

#ifdef CAGE_DEBUG
#define GCHL_EXCEPTION_GENERATE_CTOR_PARAMS const char *file, uint32 line, const char *function, severityEnum severity, const char *message
#define GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER file, line, function, severity, message
#define GCHL_EXCEPTION_GENERATE_LOG(MESSAGE) { ::cage::detail::makeLog(file, line, function, severity, "exception", MESSAGE, false); ::cage::detail::debugBreakpoint(); }
#else
#define GCHL_EXCEPTION_GENERATE_CTOR_PARAMS severityEnum severity, const char *message
#define GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER severity, message
#define GCHL_EXCEPTION_GENERATE_LOG(MESSAGE) { ::cage::detail::makeLog(severity, "exception", MESSAGE, false); ::cage::detail::debugBreakpoint(); }
#endif

namespace cage
{
	enum class severityEnum
	{
		Hint, // possible improvement available
		Note, // details for subsequent log
		Warning, // not error, but should be dealt with
		Info, // we are good
		Error, // network connection interrupted, file access denied
		Critical // not implemented function, exception inside destructor, assert failure
	};

	struct CAGE_API exception
	{
		exception(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
		virtual ~exception() noexcept;
		virtual exception &log();
#ifdef CAGE_DEBUG
		const char *file;
		const char *function;
		uint32 line;
#endif
		const char *message;
		severityEnum severity;
	};

	struct CAGE_API notImplementedException : public exception
	{
		notImplementedException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
		virtual notImplementedException &log();
	};

	struct CAGE_API utf8Exception : public exception
	{
		utf8Exception(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
		virtual utf8Exception &log();
	};

	struct CAGE_API disconnectedException : public exception
	{
		disconnectedException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
		virtual disconnectedException &log();
	};

	struct CAGE_API outOfMemoryException : public exception
	{
		outOfMemoryException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uintPtr memory) noexcept;
		virtual outOfMemoryException &log();
		uintPtr memory;
	};

	struct CAGE_API codeException : public exception
	{
		codeException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept;
		virtual codeException &log();
		uint32 code;
	};

	namespace detail
	{
		struct CAGE_API overrideException
		{
			overrideException(severityEnum severity = severityEnum::Critical);
			~overrideException();

		private:
			severityEnum original;
		};

		CAGE_API void setGlobalExceptionOverride(severityEnum severity);
		CAGE_API severityEnum getExceptionSilenceSeverity();
	}
}
