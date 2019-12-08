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

#define CAGE_THROW_SILENT(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::severityEnum::Error, __VA_ARGS__); throw e_; }
#define CAGE_THROW_ERROR(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::severityEnum::Error, __VA_ARGS__); e_.makeLog(); throw e_; }
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::severityEnum::Critical, __VA_ARGS__); e_.makeLog(); throw e_; }

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
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
	}

	namespace detail
	{
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

		struct CAGE_API overrideException
		{
			explicit overrideException(severityEnum severity = severityEnum::Critical);
			~overrideException();

		private:
			severityEnum original;
		};

		CAGE_API void setGlobalBreakpointOverride(bool enable);
		CAGE_API void setGlobalAssertOverride(bool enable);
		CAGE_API void setGlobalExceptionOverride(severityEnum severity);
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
				return variable(name, (uintPtr)var);
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
		explicit exception(const char *file, uint32 line, const char *function, severityEnum severity, const char *message) noexcept;
		virtual ~exception() noexcept;

		void makeLog();
		virtual void log();

		const char *file;
		const char *function;
		const char *message;
		uint32 line;
		severityEnum severity;
	};

	struct CAGE_API notImplemented : public exception
	{
		explicit notImplemented(const char *file, uint32 line, const char *function, severityEnum severity, const char *message) noexcept;
		virtual void log();
	};

	struct CAGE_API systemError : public exception
	{
		explicit systemError(const char *file, uint32 line, const char *function, severityEnum severity, const char *message, uint32 code) noexcept;
		virtual void log();
		uint32 code;
	};

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
	}

	// with CAGE_ASSERT_ENABLED numeric_cast validates that the value is in range of the target type, preventing overflows
	// without CAGE_ASSERT_ENABLED numeric_cast is the same as static_cast
	template<class To, class From>
	To numeric_cast(From from)
	{
		return privat::numeric_cast_helper_specialized<detail::numeric_limits<To>::is_specialized && detail::numeric_limits<From>::is_specialized>::template cast<To>(from);
	}

	// with CAGE_ASSERT_ENABLED class_cast verifies that dynamic_cast would succeed
	// without CAGE_ASSERT_ENABLED class_cast is the same as static_cast
	template<class To, class From>
	To class_cast(From from)
	{
		CAGE_ASSERT(!from || dynamic_cast<To>(from), from);
		return static_cast<To>(from);
	}
}
