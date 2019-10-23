#ifdef CAGE_DEBUG
#ifndef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT_ENABLED
#endif
#endif

#ifdef CAGE_ASSERT_ENABLED
#define GCHL_ASSVAR(VAR) .variable(CAGE_STRINGIZE(VAR), VAR)
#define CAGE_ASSERT(EXP, ...) { ::cage::privat::assertPriv(!!(EXP), CAGE_STRINGIZE(EXP), __FILE__, CAGE_STRINGIZE(__LINE__), __FUNCTION__) GCHL_DEFER(CAGE_EXPAND_ARGS(GCHL_ASSVAR, __VA_ARGS__))(); }
#else
#define CAGE_ASSERT(EXP, ...)
#endif

#define CAGE_ASSERT_COMPILE(COND, MESS) enum { CAGE_JOIN(CAGE_JOIN(gchl_assert_, MESS), CAGE_JOIN(_, __LINE__)) = 1/((int)!!(COND)) }

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
#define GCHL_EXCEPTION_GENERATE_LOG(MESSAGE) { ::cage::privat::makeLog(file, line, function, severity, "exception", (MESSAGE), false, false); ::cage::detail::debugBreakpoint(); }
#else
#define GCHL_EXCEPTION_GENERATE_CTOR_PARAMS severityEnum severity, const char *message
#define GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER severity, message
#define GCHL_EXCEPTION_GENERATE_LOG(MESSAGE) { ::cage::privat::makeLog(severity, "exception", (MESSAGE), false, false); ::cage::detail::debugBreakpoint(); }
#endif

namespace cage
{
	enum class severityEnum
	{
		Note, // details for subsequent log
		Hint, // possible improvement available
		Warning, // deprecated behavior, dangerous actions
		Info, // we are good, progress report
		Error, // invalid user input, network connection interrupted, file access denied
		Critical // not implemented function, exception inside destructor, assert failure
	};

	namespace privat
	{
		CAGE_API uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept;
		inline uint64 makeLog(severityEnum severity, const char *component, const string &message, bool continuous, bool debug) { return makeLog(nullptr, 0, nullptr, severity, component, message, continuous, debug); }
	}

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#endif

	namespace privat
	{
#define GCHL_GENERATE(TYPE) CAGE_API uint32 sprint1(char *s, TYPE value); CAGE_API void sscan1(const char *s, TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE
		CAGE_API uint32 sprint1(char *s, const void *value);
		CAGE_API uint32 sprint1(char *s, const char *value);
		CAGE_API uint32 sprint1(char *s, bool value);
	}

	namespace detail
	{
		CAGE_API void *memset(void *ptr, int value, uintPtr num);
		CAGE_API void *memcpy(void *destination, const void *source, uintPtr num);
		CAGE_API void *memmove(void *destination, const void *source, uintPtr num);
		CAGE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num);
		CAGE_API uintPtr strlen(const char *str);
		CAGE_API char *strcpy(char *destination, const char *source);
		CAGE_API char *strcat(char *destination, const char *source);
		CAGE_API int strcmp(const char *str1, const char *str2);
		CAGE_API int toupper(int c);
		CAGE_API int tolower(int c);

		CAGE_API void terminate();
		CAGE_API void debugOutput(const string &msg);
		CAGE_API void debugBreakpoint();

		struct CAGE_API overrideBreakpoint
		{
			explicit overrideBreakpoint(bool enable = false);
			~overrideBreakpoint();

		private:
			bool original;
		};

		struct CAGE_API overrideAssert
		{
			explicit overrideAssert(bool deadly = false);
			~overrideAssert();

		private:
			bool original;
		};

		CAGE_API void setGlobalBreakpointOverride(bool enable);
		CAGE_API void setGlobalAssertOverride(bool enable);
	}

	namespace privat
	{
		struct CAGE_API assertPriv
		{
			explicit assertPriv(bool exp, const char *expt, const char *file, const char *line, const char *function);
			void operator () () const;

#define GCHL_GENERATE(TYPE) assertPriv &variable(const char *name, TYPE var);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, bool, float, double, const char*, \
				const string&, real, rads, degs, const vec2&, const vec3&, const vec4&, const quat&, const mat3&, const mat4&));
#undef GCHL_GENERATE

			template<class U>
			assertPriv &variable(const char *name, U *var)
			{
				if (!valid)
				{
					char buff[100] = "";
					privat::sprint1(buff, (void*)var);
					detail::strcat(buff, " <pointer>");
					format(name, buff);
				}
				return *this;
			}

			template<class U>
			assertPriv &variable(const char *name, const U &var)
			{
				if (!valid)
					format(name, "<unknown variable type>");
				return *this;
			}

		private:
			const bool valid;
			void format(const char *name, const char *var) const;
		};
	}

	struct CAGE_API exception
	{
		explicit exception(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
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

	struct CAGE_API notImplemented : public exception
	{
		explicit notImplemented(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
		virtual notImplemented &log();
	};

	struct CAGE_API outOfMemory : public exception
	{
		explicit outOfMemory(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uintPtr memory) noexcept;
		virtual outOfMemory &log();
		uintPtr memory;
	};

	struct CAGE_API codeException : public exception
	{
		explicit codeException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS, uint32 code) noexcept;
		virtual codeException &log();
		uint32 code;
	};

	namespace detail
	{
		struct CAGE_API overrideException
		{
			explicit overrideException(severityEnum severity = severityEnum::Critical);
			~overrideException();

		private:
			severityEnum original;
		};

		CAGE_API void setGlobalExceptionOverride(severityEnum severity);
		CAGE_API severityEnum getExceptionSilenceSeverity();
	}

	CAGE_API uint64 getApplicationTime();

	namespace privat
	{
		template<bool ToSig, bool FromSig>
		struct numeric_cast_helper_signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT(from >= detail::numeric_limits<To>::min(), "numeric cast failure", from, detail::numeric_limits<To>::min());
				CAGE_ASSERT(from <= detail::numeric_limits<To>::max(), "numeric cast failure", from, detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<false, true> // signed -> unsigned
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT(from >= 0, "numeric cast failure", from);
				typedef typename detail::numeric_limits<From>::make_unsigned unsgFrom;
				CAGE_ASSERT(static_cast<unsgFrom>(from) <= detail::numeric_limits<To>::max(), "numeric cast failure", from, static_cast<unsgFrom>(from), detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<true, false> // unsigned -> signed
		{
			template<class To, class From>
			static To cast(From from)
			{
				typedef typename detail::numeric_limits<To>::make_unsigned unsgTo;
				CAGE_ASSERT(from <= static_cast<unsgTo>(detail::numeric_limits<To>::max()), "numeric cast failure", from, detail::numeric_limits<To>::max(), static_cast<unsgTo>(detail::numeric_limits<To>::max()));
				return static_cast<To>(from);
			}
		};

		template<bool Specialized>
		struct numeric_cast_helper_specialized
		{};

		template<>
		struct numeric_cast_helper_specialized<true>
		{
			template<class To, class From>
			static To cast(From from)
			{
				return numeric_cast_helper_signed<detail::numeric_limits<To>::is_signed, detail::numeric_limits<From>::is_signed>::template cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_specialized<false>
		{
			template<class To, class From>
			static To cast(From from)
			{
				CAGE_ASSERT_COMPILE(false, numeric_cast_is_allowed_for_numbers_only);
				return 0;
			}
		};
	}

	template<class To, class From>
	To numeric_cast(From from)
	{
		return privat::numeric_cast_helper_specialized<detail::numeric_limits<To>::is_specialized && detail::numeric_limits<From>::is_specialized>::template cast<To>(from);
	}

	template<class To, class From>
	To class_cast(From from)
	{
		CAGE_ASSERT(dynamic_cast<To>(from), from);
		return static_cast<To>(from);
	}
}
