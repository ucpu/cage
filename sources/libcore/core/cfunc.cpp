#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>

namespace cage
{
	namespace detail
	{
		void *memset(void *ptr, int value, uintPtr num)
		{
			return ::memset(ptr, value, num);
		}

		void *memcpy(void *destination, const void *source, uintPtr num)
		{
			return ::memcpy(destination, source, num);
		}

		void *memmove(void *destination, const void *source, uintPtr num)
		{
			return ::memmove(destination, source, num);
		}

		int memcmp(const void *ptr1, const void *ptr2, uintPtr num)
		{
			return ::memcmp(ptr1, ptr2, num);
		}

		uintPtr strlen(const char *str)
		{
			return ::strlen(str);
		}

		int strcmp(const char *str1, const char *str2)
		{
			return ::strcmp(str1, str2);
		}

		int toupper(int c)
		{
			return ::toupper(c);
		}

		int tolower(int c)
		{
			return ::tolower(c);
		}

		char *strcpy(char *destination, const char *source)
		{
			return ::strcpy(destination, source);
		}

		char *strcat(char *destination, const char *source)
		{
			return ::strcat(destination, source);
		}
	}

	namespace privat
	{
		namespace
		{
			template<class T> void genericScan(const char *s, T &value)
			{
				errno = 0;
				char *e = nullptr;
				if (detail::numeric_limits<T>::is_signed)
				{
					sint64 v = strtoll(s, &e, 10);
					value = (T)v;
					if (v < detail::numeric_limits<T>::min() || v > detail::numeric_limits<T>::max())
						e = nullptr;
				}
				else
				{
					uint64 v = strtoull(s, &e, 10);
					value = (T)v;
					if (*s == '-' || v < detail::numeric_limits<T>::min() || v > detail::numeric_limits<T>::max())
						e = nullptr;
				}
				if (!e || *e != 0 || isspace(*s) || errno != 0)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(exception, "sscan1 failed");
				}
			}

			template<> void genericScan<sint64>(const char *s, sint64 &value)
			{
				errno = 0;
				char *e = nullptr;
				value = strtoll(s, &e, 10);
				if (!e || *e != 0 || isspace(*s) || errno != 0)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(exception, "sscan1 failed");
				}
			}

			template<> void genericScan<uint64>(const char *s, uint64 &value)
			{
				errno = 0;
				char *e = nullptr;
				value = strtoull(s, &e, 10);
				if (!e || *s == '-' || *e != 0 || isspace(*s) || errno != 0)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(exception, "sscan1 failed");
				}
			}

			template<> void genericScan<double>(const char *s, double &value)
			{
				errno = 0;
				char *e = nullptr;
				double v = strtod(s, &e);
				if (!e || *e != 0 || isspace(*s) || errno != 0)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(exception, "sscan1 failed");
				}
				value = v;
			}

			template<> void genericScan<float>(const char *s, float &value)
			{
				double v;
				genericScan(s, v);
				if (v < detail::numeric_limits<float>::min() || v > detail::numeric_limits<float>::max())
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(exception, "sscan1 failed");
				}
				value = (float)v;
			}
		}

#define GCHL_GENERATE(TYPE, SPEC) \
		uint32 sprint1(char *s, TYPE value) \
		{ sint32 ret = sprintf(s, CAGE_STRINGIZE(SPEC), value); if (ret < 0) CAGE_THROW_ERROR(exception, "sprint1 failed"); return ret; } \
		void sscan1(const char *s, TYPE &value) \
		{ return genericScan(s, value); }
		GCHL_GENERATE(sint8, %hhd);
		GCHL_GENERATE(sint16, %hd);
		GCHL_GENERATE(sint32, %d);
		GCHL_GENERATE(uint8, %hhu);
		GCHL_GENERATE(uint16, %hu);
		GCHL_GENERATE(uint32, %u);
#ifdef CAGE_SYSTEM_WINDOWS
		GCHL_GENERATE(sint64, %lld);
		GCHL_GENERATE(uint64, %llu);
#else
		GCHL_GENERATE(sint64, %ld);
		GCHL_GENERATE(uint64, %lu);
#endif
		GCHL_GENERATE(float, %f);
		GCHL_GENERATE(double, %lf);
#undef GCHL_GENERATE

		uint32 sprint1(char *s, const void *value)
		{
			return sprint1(s, (uintPtr)value);
		}

		uint32 sprint1(char *s, const char *value)
		{
			sint32 ret = sprintf(s, "%s", value); if (ret < 0) CAGE_THROW_ERROR(exception, "sprint1 failed"); return ret;
		}

		uint32 sprint1(char *s, bool value)
		{
			return sprint1(s, value ? "true" : "false");
		}
	}
}
