#ifdef CAGE_DEBUG
#ifndef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT_ENABLED
#endif
#endif

#ifdef CAGE_ASSERT_ENABLED
#define GCHL_ASSVAR(VAR) .variable(CAGE_STRINGIZE(VAR), VAR)
#define CAGE_ASSERT_RUNTIME(EXP, ...) { ::cage::privat::assertClass(!!(EXP), CAGE_STRINGIZE(EXP), __FILE__, CAGE_STRINGIZE(__LINE__), __FUNCTION__) GCHL_DEFER(CAGE_EXPAND_ARGS(GCHL_ASSVAR, __VA_ARGS__))(); }
#else
#define CAGE_ASSERT_RUNTIME(EXP, ...)
#endif

#define CAGE_ASSERT_COMPILE(COND, MESS) enum { CAGE_JOIN(CAGE_JOIN(cage_assert_, MESS), CAGE_JOIN(_, __LINE__)) = 1/((int)!!(COND)) }

namespace cage
{
	namespace detail
	{
		CAGE_API void terminate();
		CAGE_API void debugOutput(const char *value);
		CAGE_API void debugBreakpoint();

		struct CAGE_API overrideBreakpoint
		{
			overrideBreakpoint(bool enable = false);
			~overrideBreakpoint();

		private:
			bool original;
		};

		struct CAGE_API overrideAssert
		{
			overrideAssert(bool deadly = false);
			~overrideAssert();

		private:
			bool original;
		};

		CAGE_API void setGlobalBreakpointOverride(bool enable);
		CAGE_API void setGlobalAssertOverride(bool enable);
	}

	namespace privat
	{
		struct CAGE_API assertClass
		{
			assertClass(bool exp, const char *expt, const char *file, const char *line, const char *function);
			void operator () () const;

#define GCHL_GENERATE(TYPE) assertClass &variable(const char *name, TYPE var);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, bool, float, double, const char*, \
				pointer, const string&, real, rads, degs, const vec2&, const vec3&, const vec4&, const quat&, const mat3&, const mat4&));
#undef GCHL_GENERATE
			template<class U> assertClass &variable(const char *name, U *var)
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
			template<class U> assertClass &variable(const char *name, const U &var)
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
}
