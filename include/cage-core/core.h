#ifndef guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
#define guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_

#if !defined(CAGE_DEBUG) && !defined(NDEBUG)
#define CAGE_DEBUG
#endif
#if defined(CAGE_DEBUG) && defined(NDEBUG)
#error CAGE_DEBUG and NDEBUG are incompatible
#endif

#if defined(_WIN32)
#define CAGE_API_EXPORT __declspec(dllexport)
#define CAGE_API_IMPORT __declspec(dllimport)
#else
#define CAGE_API_EXPORT __attribute__((visibility("default")))
#define CAGE_API_IMPORT __attribute__((visibility("default")))
#endif

#ifdef CAGE_CORE_EXPORT
#define CAGE_CORE_API CAGE_API_EXPORT
#else
#define CAGE_CORE_API CAGE_API_IMPORT
#endif

#if defined(_WIN32) || defined(__WIN32__)
#define CAGE_SYSTEM_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__)
#define CAGE_SYSTEM_LINUX
#elif defined(__APPLE__)
#define CAGE_SYSTEM_MAC
#else
#error This operating system is not supported
#endif

#ifdef CAGE_SYSTEM_WINDOWS
#define CAGE_API_PRIVATE
#if defined(_WIN64)
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif
#else
#define CAGE_API_PRIVATE __attribute__((visibility("hidden")))
#if __x86_64__ || __ppc64__
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif
#endif

#ifdef CAGE_DEBUG
#ifndef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT_ENABLED
#endif
#endif
#ifdef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT(EXPR) { if (!(EXPR)) ::cage::privat::runtimeAssertFailure(#EXPR, __FILE__, __LINE__, __FUNCTION__); }
#else
#define CAGE_ASSERT(EXPR) {}
#endif

#define CAGE_THROW_SILENT(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Error, __VA_ARGS__); throw e_; }
#define CAGE_THROW_ERROR(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Error, __VA_ARGS__); e_.makeLog(); throw e_; }
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) { EXCEPTION e_(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Critical, __VA_ARGS__); e_.makeLog(); throw e_; }

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
	// numeric types

	typedef unsigned char uint8;
	typedef signed char sint8;
	typedef unsigned short uint16;
	typedef signed short sint16;
	typedef unsigned int uint32;
	typedef signed int sint32;
#ifdef CAGE_SYSTEM_WINDOWS
	typedef unsigned long long uint64;
	typedef signed long long sint64;
#else
	typedef unsigned long uint64;
	typedef signed long sint64;
#endif
#ifdef CAGE_ARCHITECTURE_64
	typedef uint64 uintPtr;
	typedef sint64 sintPtr;
#else
	typedef uint32 uintPtr;
	typedef sint32 sintPtr;
#endif

	// forward declare string

	namespace detail { template<uint32 N> struct StringBase; }
	typedef detail::StringBase<1000> string;

	// severity, makeLog, runtimeAssertFailure

	enum class SeverityEnum
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
		CAGE_CORE_API uint64 makeLog(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept;
		CAGE_CORE_API void runtimeAssertFailure(const char *expt, const char *file, uintPtr line, const char *function);
	}

	// numeric limits

	namespace privat
	{
		template<bool> struct helper_is_char_signed {};
		template<> struct helper_is_char_signed<true> { typedef signed char type; };
		template<> struct helper_is_char_signed<false> { typedef unsigned char type; };
	}

	namespace detail
	{
		template<class T>
		struct numeric_limits
		{};

		template<>
		struct numeric_limits<unsigned char>
		{
			static constexpr bool is_signed = false;
			static constexpr uint8 min() noexcept { return 0; };
			static constexpr uint8 max() noexcept { return 255u; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned short>
		{
			static constexpr bool is_signed = false;
			static constexpr uint16 min() noexcept { return 0; };
			static constexpr uint16 max() noexcept { return 65535u; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned int>
		{
			static constexpr bool is_signed = false;
			static constexpr uint32 min() noexcept { return 0; };
			static constexpr uint32 max() noexcept { return 4294967295u; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned long long>
		{
			static constexpr bool is_signed = false;
			static constexpr uint64 min() noexcept { return 0; };
			static constexpr uint64 max() noexcept { return 18446744073709551615LLu; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned int>
		{};
#else
		template<>
		struct numeric_limits<unsigned long> : public numeric_limits<unsigned long long>
		{};
#endif

		template<>
		struct numeric_limits<signed char>
		{
			static constexpr bool is_signed = true;
			static constexpr sint8 min() noexcept { return -127 - 1; };
			static constexpr sint8 max() noexcept { return  127; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<signed short>
		{
			static constexpr bool is_signed = true;
			static constexpr sint16 min() noexcept { return -32767 - 1; };
			static constexpr sint16 max() noexcept { return  32767; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<signed int>
		{
			static constexpr bool is_signed = true;
			static constexpr sint32 min() noexcept { return -2147483647 - 1; };
			static constexpr sint32 max() noexcept { return  2147483647; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<signed long long>
		{
			static constexpr bool is_signed = true;
			static constexpr sint64 min() noexcept { return -9223372036854775807LL - 1; };
			static constexpr sint64 max() noexcept { return  9223372036854775807LL; };
			typedef uint64 make_unsigned;
		};

#ifdef CAGE_SYSTEM_WINDOWS
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed int>
		{};
#else
		template<>
		struct numeric_limits<signed long> : public numeric_limits<signed long long>
		{};
#endif

		template<>
		struct numeric_limits<float>
		{
			static constexpr bool is_signed = true;
			static constexpr float min() noexcept { return -1e+37f; };
			static constexpr float max() noexcept { return  1e+37f; };
			typedef float make_unsigned;
		};

		template<>
		struct numeric_limits<double>
		{
			static constexpr bool is_signed = true;
			static constexpr double min() noexcept { return -1e+308; };
			static constexpr double max() noexcept { return  1e+308; };
			typedef double make_unsigned;
		};

		// char is distinct type from both unsigned char and signed char
		template<>
		struct numeric_limits<char> : public numeric_limits<privat::helper_is_char_signed<(char)-1 < (char)0>::type>
		{};
	}

	// numeric cast

	namespace privat
	{
		template<bool ToSig, bool FromSig>
		struct numeric_cast_helper_signed
		{
			template<class To, class From>
			static constexpr To cast(From from)
			{
				CAGE_ASSERT(from >= detail::numeric_limits<To>::min());
				CAGE_ASSERT(from <= detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<false, true> // signed -> unsigned
		{
			template<class To, class From>
			static constexpr To cast(From from)
			{
				CAGE_ASSERT(from >= 0);
				typedef typename detail::numeric_limits<From>::make_unsigned unsgFrom;
				CAGE_ASSERT(static_cast<unsgFrom>(from) <= detail::numeric_limits<To>::max());
				return static_cast<To>(from);
			}
		};

		template<>
		struct numeric_cast_helper_signed<true, false> // unsigned -> signed
		{
			template<class To, class From>
			static constexpr To cast(From from)
			{
				typedef typename detail::numeric_limits<To>::make_unsigned unsgTo;
				CAGE_ASSERT(from <= static_cast<unsgTo>(detail::numeric_limits<To>::max()));
				return static_cast<To>(from);
			}
		};
	}

	// with CAGE_ASSERT_ENABLED numeric_cast validates that the value is in range of the target type, preventing overflows
	// without CAGE_ASSERT_ENABLED numeric_cast is the same as static_cast
	template<class To, class From>
	inline constexpr To numeric_cast(From from)
	{
		return privat::numeric_cast_helper_signed<detail::numeric_limits<To>::is_signed, detail::numeric_limits<From>::is_signed>::template cast<To>(from);
	}

	// with CAGE_ASSERT_ENABLED class_cast verifies that dynamic_cast would succeed
	// without CAGE_ASSERT_ENABLED class_cast is the same as static_cast
	template<class To, class From>
	inline constexpr To class_cast(From from)
	{
		CAGE_ASSERT(!from || dynamic_cast<To>(from) == static_cast<To>(from));
		return static_cast<To>(from);
	}

	// max struct

	namespace privat
	{
		// represents the maximum positive value possible in any numeric type
		struct MaxValue
		{
			template<class T>
			constexpr operator T () const noexcept
			{
				return detail::numeric_limits<T>::max();
			}
		};

		template<class T> inline constexpr bool operator == (T lhs, MaxValue rhs) noexcept { return lhs == detail::numeric_limits<T>::max(); }
		template<class T> inline constexpr bool operator != (T lhs, MaxValue rhs) noexcept { return lhs != detail::numeric_limits<T>::max(); }
		template<class T> inline constexpr bool operator <= (T lhs, MaxValue rhs) noexcept { return lhs <= detail::numeric_limits<T>::max(); }
		template<class T> inline constexpr bool operator >= (T lhs, MaxValue rhs) noexcept { return lhs >= detail::numeric_limits<T>::max(); }
		template<class T> inline constexpr bool operator <  (T lhs, MaxValue rhs) noexcept { return lhs <  detail::numeric_limits<T>::max(); }
		template<class T> inline constexpr bool operator >  (T lhs, MaxValue rhs) noexcept { return lhs >  detail::numeric_limits<T>::max(); }
	}

	static constexpr privat::MaxValue m = privat::MaxValue();

	// template magic

	namespace templates
	{
		template<class T>
		struct Comparable
		{
			friend bool operator == (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) == 0; }
			friend bool operator != (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) != 0; }
			friend bool operator <= (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) <= 0; }
			friend bool operator >= (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) >= 0; }
			friend bool operator <  (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) <  0; }
			friend bool operator >  (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) >  0; }
		};
		template<bool Cond, class T> struct enable_if {};
		template<class T> struct enable_if<true, T> { typedef T type; };
		template<uintPtr Size> struct base_unsigned_type {};
		template<> struct base_unsigned_type<1> { typedef uint8 type; };
		template<> struct base_unsigned_type<2> { typedef uint16 type; };
		template<> struct base_unsigned_type<4> { typedef uint32 type; };
		template<> struct base_unsigned_type<8> { typedef uint64 type; };
		template<class T> struct underlying_type { typedef typename base_unsigned_type<sizeof(T)>::type type; };
		template<class T> struct remove_const { typedef T type; };
		template<class T> struct remove_const<const T> { typedef T type; };
		template<class T> struct remove_reference { typedef T type; };
		template<class T> struct remove_reference<T&> { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };
		template<typename T> struct is_lvalue_reference { static constexpr bool value = false; };
		template<typename T> struct is_lvalue_reference<T&> { static constexpr bool value = true; };
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type  &v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type &&v) noexcept { static_assert(!is_lvalue_reference<T>::value, "invalid rvalue to lvalue conversion"); return static_cast<T&&>(v); }
		template<class T> inline constexpr typename remove_reference<T>::type &&move(T &&v) noexcept { return static_cast<typename remove_reference<T>::type&&>(v); }
	}

	// enum class bit operators

	template<class T> struct enable_bitmask_operators { static constexpr bool enable = false; };
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ~ (T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(~static_cast<underlying>(lhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator | (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator & (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^ (T lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator |= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator &= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^= (T &lhs, T rhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type any(T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) != 0; }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type none(T lhs) noexcept { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) == 0; }

	// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(TYPE) template<> struct enable_bitmask_operators<TYPE> { static constexpr bool enable = true; };

	// Immovable

	struct CAGE_CORE_API Immovable
	{
		Immovable() = default;
		Immovable(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator = (const Immovable &) = delete;
		Immovable &operator = (Immovable &&) = delete;
	};

	// exceptions

	struct CAGE_CORE_API Exception
	{
		explicit Exception(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept;
		virtual ~Exception() noexcept;

		void makeLog(); // check conditions and call log()
		virtual void log();

		const char *file = nullptr;
		const char *function = nullptr;
		const char *message = nullptr;
		uint32 line = 0;
		SeverityEnum severity = SeverityEnum::Critical;
	};

	struct CAGE_CORE_API NotImplemented : public Exception
	{
		explicit NotImplemented(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept;
		void log() override;
	};

	struct CAGE_CORE_API SystemError : public Exception
	{
		explicit SystemError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
		void log() override;
		uint32 code = 0;
	};

	// forward declarations

	struct real;
	struct rads;
	struct degs;
	struct vec2;
	struct vec3;
	struct vec4;
	struct quat;
	struct mat3;
	struct mat4;
	struct transform;
	struct line;
	struct triangle;
	struct plane;
	struct sphere;
	struct aabb;

	class AssetManager;
	struct AssetManagerCreateConfig;
	struct AssetContext;
	struct AssetScheme;
	struct AssetHeader;
	struct AssetHeader;
	enum class StereoModeEnum : uint32;
	enum class StereoEyeEnum : uint32;
	struct StereoCameraInput;
	struct StereoCameraOutput;
	class CollisionMesh;
	struct CollisionPair;
	class CollisionQuery;
	class CollisionStructure;
	struct CollisionStructureCreateConfig;
	class Mutex;
	class RwMutex;
	class Barrier;
	class Semaphore;
	class ConditionalVariableBase;
	class ConditionalVariable;
	class Thread;
	struct ConcurrentQueueTerminated;
	template<class T> class ConcurrentQueue;
	enum class ConfigTypeEnum : uint32;
	struct ConfigBool;
	struct ConfigSint32;
	struct ConfigSint64;
	struct ConfigUint32;
	struct ConfigUint64;
	struct ConfigFloat;
	struct ConfigDouble;
	struct ConfigString;
	class ConfigList;
	struct EntityComponentCreateConfig;
	class EntityManager;
	struct EntityManagerCreateConfig;
	class Entity;
	class EntityComponent;
	class EntityGroup;
	struct FileMode;
	class File;
	enum class PathTypeFlags : uint32;
	class FilesystemWatcher;
	class DirectoryList;
	template<uint32 N> struct Guid;
	struct HashString;
	enum class ImageFormatEnum : uint32;
	enum class GammaSpaceEnum : uint32;
	enum class AlphaModeEnum : uint32;
	struct ImageColorConfig;
	class Image;
	class Ini;
	class LineReader;
	class Logger;
	class LoggerOutputFile;
	template<class Key, class Value, class Hasher> struct LruCache;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyLinear;
	template<uint8 N, class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyNFrame;
	template<uintPtr AtomSize, class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyPool;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyQueue;
	template<class BoundsPolicy, class TaggingPolicy, class TrackingPolicy> struct MemoryAllocatorPolicyStack;
	template<class AllocatorPolicy, class ConcurrentPolicy> struct MemoryArenaFixed;
	template<class AllocatorPolicy, class ConcurrentPolicy> struct MemoryArenaGrowing;
	template<class T> struct MemoryArenaStd;
	struct MemoryBoundsPolicyNone;
	struct MemoryBoundsPolicySimple;
	struct MemoryTagPolicyNone;
	struct MemoryTagPolicySimple;
	struct MemoryTrackPolicyNone;
	struct MemoryTrackPolicySimple;
	struct MemoryTrackPolicyAdvanced;
	struct MemoryConcurrentPolicyNone;
	struct MemoryConcurrentPolicyMutex;
	struct OutOfMemory;
	class VirtualMemory;
	struct MemoryBuffer;
	struct Disconnected;
	class TcpConnection;
	class TcpServer;
	struct UdpStatistics;
	class UdpConnection;
	class UdpServer;
	struct DiscoveryPeer;
	class DiscoveryClient;
	class DiscoveryServer;
	enum class NoiseTypeEnum : uint32;
	enum class NoiseInterpolationEnum : uint32;
	enum class NoiseFractalTypeEnum : uint32;
	enum class NoiseDistanceEnum : uint32;
	enum class NoiseOperationEnum : uint32;
	class NoiseFunction;
	struct NoiseFunctionCreateConfig;
	template<class T> struct PointerRangeHolder;
	struct ProcessCreateConfig;
	class Process;
	struct RandomGenerator;
	enum class ScheduleTypeEnum : uint32;
	struct ScheduleCreateConfig;
	class Schedule;
	struct SchedulerCreateConfig;
	class Scheduler;
	template<class T> struct ScopeLock;
	struct Serializer;
	struct Deserializer;
	class SpatialQuery;
	class SpatialStructure;
	struct SpatialStructureCreateConfig;
	class BufferIStream;
	class BufferOStream;
	class SwapBufferGuard;
	struct SwapBufferGuardCreateConfig;
	class TextPack;
	class ThreadPool;
	class Timer;
	struct InvalidUtfString;
	template<class T, class F> struct VariableInterpolatingBuffer;
	template<class T, uint32 N = 16> struct VariableSmoothingBuffer;

	// string

	namespace privat
	{
#define GCHL_GENERATE(TYPE) \
		CAGE_CORE_API uint32 toString(char *s, TYPE value); \
		CAGE_CORE_API void fromString(const char *s, TYPE &value);
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
		GCHL_GENERATE(bool);
#undef GCHL_GENERATE
		CAGE_CORE_API uint32 toString(char *dst, uint32 dstLen, const char *src);

		CAGE_CORE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen);
		CAGE_CORE_API void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right);
		CAGE_CORE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen);
		CAGE_CORE_API uint32 stringToUpper(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_CORE_API uint32 stringToLower(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_CORE_API uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset) noexcept;
		CAGE_CORE_API void stringEncodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API void stringDecodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API bool stringIsDigitsOnly(const char *data, uint32 dataLen) noexcept;
		CAGE_CORE_API bool stringIsInteger(const char *data, uint32 dataLen, bool allowSign) noexcept;
		CAGE_CORE_API bool stringIsReal(const char *data, uint32 dataLen, bool allowSign) noexcept;
		CAGE_CORE_API bool stringIsBool(const char *data, uint32 dataLen) noexcept;
		CAGE_CORE_API bool stringIsPattern(const char *data, uint32 dataLen, const char *prefix, uint32 prefixLen, const char *infix, uint32 infixLen, const char *suffix, uint32 suffixLen) noexcept;
		CAGE_CORE_API int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl) noexcept;
	}

	namespace detail
	{
		CAGE_CORE_API void *memset(void *destination, int value, uintPtr num) noexcept;
		CAGE_CORE_API void *memcpy(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API void *memmove(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num) noexcept;

		template<uint32 N>
		struct StringBase : templates::Comparable<StringBase<N>>
		{
			// constructors
			StringBase()
			{
				data[current] = 0;
			}

			StringBase(const StringBase &other) noexcept : current(other.current)
			{
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

			template<uint32 M>
			StringBase(const StringBase<M> &other)
			{
				if (other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
			}

#define GCHL_GENERATE(TYPE) \
			explicit StringBase(TYPE other) \
			{ \
				static_assert(N >= 20, "string too short"); \
				current = privat::toString(data, other); \
			}
			GCHL_GENERATE(sint8);
			GCHL_GENERATE(sint16);
			GCHL_GENERATE(sint32);
			GCHL_GENERATE(sint64);
			GCHL_GENERATE(uint8);
			GCHL_GENERATE(uint16);
			GCHL_GENERATE(uint32);
			GCHL_GENERATE(uint64);
			GCHL_GENERATE(float);
			GCHL_GENERATE(double);
			GCHL_GENERATE(bool);
#undef GCHL_GENERATE

			/*
			template<class T>
			explicit StringBase(T *other) : StringBase((uintPtr)other)
			{}
			*/

			explicit StringBase(const char *pos, uint32 len)
			{
				if (len > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = len;
				detail::memcpy(data, pos, len);
				data[current] = 0;
			}

			explicit StringBase(char *other)
			{
				current = privat::toString(data, N, other);
			}

			StringBase(const char *other)
			{
				current = privat::toString(data, N, other);
			}

			// assignment operators
			StringBase &operator = (const StringBase &other) noexcept
			{
				if (this == &other)
					return *this;
				current = other.current;
				detail::memcpy(data, other.data, current);
				data[current] = 0;
				return *this;
			}

			// compound operators
			StringBase &operator += (const StringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				detail::memcpy(data + current, other.data, other.current);
				current += other.current;
				data[current] = 0;
				return *this;
			}

			// binary operators
			StringBase operator + (const StringBase &other) const
			{
				return StringBase(*this) += other;
			}

			char &operator [] (uint32 idx)
			{
				CAGE_ASSERT(idx < current);
				return data[idx];
			}

			char operator [] (uint32 idx) const
			{
				CAGE_ASSERT(idx < current);
				return data[idx];
			}

			// methods
			const char *c_str() const
			{
				CAGE_ASSERT(data[current] == 0);
				return data;
			}

			StringBase reverse() const
			{
				StringBase ret = *this;
				for (uint32 i = 0; i < current; i++)
					ret.data[current - i - 1] = data[i];
				return ret;
			}

			StringBase subString(uint32 start, uint32 length) const
			{
				if (start >= current)
					return "";
				uint32 len = length;
				if (length == m || start + length > current)
					len = current - start;
				return StringBase(data + start, len);
			}

			StringBase replace(const StringBase &what, const StringBase &with) const
			{
				StringBase ret = *this;
				privat::stringReplace(ret.data, ret.current, N, what.data, what.current, with.data, with.current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase replace(uint32 start, uint32 length, const StringBase &with) const
			{
				return subString(0, start) + with + subString(start + length, m);
			}

			StringBase remove(uint32 start, uint32 length) const
			{
				return subString(0, start) + subString(start + length, m);
			}

			StringBase insert(uint32 start, const StringBase &what) const
			{
				return subString(0, start) + what + subString(start, m);
			}

			StringBase trim(bool left = true, bool right = true, const StringBase &trimChars = "\t\n ") const
			{
				StringBase ret = *this;
				privat::stringTrim(ret.data, ret.current, trimChars.data, trimChars.current, left, right);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase split(const StringBase &splitChars = "\t\n ")
			{
				StringBase ret;
				privat::stringSplit(data, current, ret.data, ret.current, splitChars.data, splitChars.current);
				data[current] = 0;
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase fill(uint32 size, char c = ' ') const
			{
				StringBase cc(&c, 1);
				StringBase ret = *this;
				while (ret.length() < size)
					ret += cc;
				ret.data[ret.current] = 0;
				return ret;
			}

			uint32 find(const StringBase &other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, other.data, other.current, offset);
			}

			uint32 find(char other, uint32 offset = 0) const
			{
				return privat::stringFind(data, current, &other, 1, offset);
			}

			StringBase encodeUrl() const
			{
				StringBase ret;
				privat::stringEncodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase decodeUrl() const
			{
				StringBase ret;
				privat::stringDecodeUrl(data, current, ret.data, ret.current, N);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toUpper() const
			{
				StringBase ret;
				ret.current = privat::stringToUpper(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			StringBase toLower() const
			{
				StringBase ret;
				ret.current = privat::stringToLower(ret.data, N, data, current);
				ret.data[ret.current] = 0;
				return ret;
			}

			float toFloat() const
			{
				float i;
				privat::fromString(data, i);
				return i;
			}

			double toDouble() const
			{
				double i;
				privat::fromString(data, i);
				return i;
			}

			sint32 toSint32() const
			{
				sint32 i;
				privat::fromString(data, i);
				return i;
			}

			uint32 toUint32() const
			{
				uint32 i;
				privat::fromString(data, i);
				return i;
			}

			sint64 toSint64() const
			{
				sint64 i;
				privat::fromString(data, i);
				return i;
			}

			uint64 toUint64() const
			{
				uint64 i;
				privat::fromString(data, i);
				return i;
			}

			bool toBool() const
			{
				bool i;
				privat::fromString(data, i);
				return i;
			}

			bool isPattern(const StringBase &prefix, const StringBase &infix, const StringBase &suffix) const
			{
				return privat::stringIsPattern(data, current, prefix.data, prefix.current, infix.data, infix.current, suffix.data, suffix.current);
			}

			bool isDigitsOnly() const
			{
				return privat::stringIsDigitsOnly(data, current);
			}

			bool isInteger(bool allowSign) const
			{
				return privat::stringIsInteger(data, current, allowSign);
			}

			bool isReal(bool allowSign) const
			{
				return privat::stringIsReal(data, current, allowSign);
			}

			bool isBool() const
			{
				return privat::stringIsBool(data, current);
			}

			bool empty() const
			{
				return current == 0;
			}

			uint32 length() const
			{
				return current;
			}

			uint32 size() const
			{
				return current;
			}

			char *begin()
			{
				return data;
			}

			char *end()
			{
				return data + current;
			}

			const char *begin() const
			{
				return data;
			}

			const char *end() const
			{
				return data + current;
			}

			static constexpr uint32 MaxLength = N;

		private:
			char data[N + 1];
			uint32 current = 0;

			template<uint32 M>
			friend struct StringBase;

			friend int compare(const StringBase &a, const StringBase &b)
			{
				return privat::stringComparison(a.data, a.current, b.data, b.current);
			}
		};

		template<uint32 N>
		struct StringizerBase
		{
			StringBase<N> value;

			operator const StringBase<N> & () const
			{
				return value;
			}

			template<uint32 M>
			operator const StringBase<M> () const
			{
				return value;
			}
		};

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringizerBase<M> &other)
		{
			str.value += other.value;
			return str;
		}

		template<uint32 N, uint32 M>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const StringBase<M> &other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, const char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N>
		StringizerBase<N> &operator + (StringizerBase<N> &str, char *other)
		{
			str.value += other;
			return str;
		}

		template<uint32 N, class T>
		StringizerBase<N> &operator + (StringizerBase<N> &str, T *other)
		{
			return str + (uintPtr)other;
		}

#define GCHL_GENERATE(TYPE) \
		template<uint32 N> \
		inline StringizerBase<N> &operator + (StringizerBase<N> &str, TYPE other) \
		{ \
			return str + StringBase<20>(other); \
		}
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
		GCHL_GENERATE(bool);
#undef GCHL_GENERATE

		template<uint32 N, class T>
		inline StringizerBase<N> &operator + (StringizerBase<N> &&str, const T &other)
		{
			// allow to use l-value-reference operator overloads with r-value-reference stringizer
			return str + other;
		}

		template<uint32 N>
		struct StringComparatorFastBase
		{
			bool operator () (const StringBase<N> &a, const StringBase<N> &b) const noexcept
			{
				if (a.length() == b.length())
					return detail::memcmp(a.begin(), b.begin(), a.length()) < 0;
				return a.length() < b.length();
			}
		};
	}

	typedef detail::StringBase<1000> string;
	typedef detail::StringizerBase<1000> stringizer;
	typedef detail::StringComparatorFastBase<1000> stringComparatorFast;

	// delegates

	template<class T>
	struct Delegate;

	template<class R, class... Ts>
	struct Delegate<R(Ts...)>
	{
	private:
		R(*fnc)(void *, Ts...) = nullptr;
		void *inst = nullptr;

	public:
		template<R(*F)(Ts...)>
		Delegate &bind() noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				(void)inst;
				return F(templates::forward<Ts>(vs)...);
			};
			return *this;
		}

		template<class D, R(*F)(D, Ts...)>
		Delegate &bind(D d) noexcept
		{
			static_assert(sizeof(d) <= sizeof(inst), "invalid size of data stored in delegate");
			union U
			{
				void *p;
				D d;
			};
			fnc = +[](void *inst, Ts... vs) {
				U u;
				u.p = inst;
				return F(u.d , templates::forward<Ts>(vs)...);
			};
			U u;
			u.d = d;
			inst = u.p;
			return *this;
		}

		template<class C, R(C::*F)(Ts...)>
		Delegate &bind(C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = i;
			return *this;
		}

		template<class C, R(C::*F)(Ts...) const>
		Delegate &bind(const C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((const C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = const_cast<C*>(i);
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return !!fnc;
		}

		void clear() noexcept
		{
			inst = nullptr;
			fnc = nullptr;
		}

		R operator ()(Ts... vs) const
		{
			CAGE_ASSERT(!!*this);
			return fnc(inst, templates::forward<Ts>(vs)...);
		}

		bool operator == (const Delegate &other) const noexcept
		{
			return fnc == other.fnc && inst == other.inst;
		}

		bool operator != (const Delegate &other) const noexcept
		{
			return !(*this == other);
		}
	};

	// holder

	template<class T> struct Holder;

	namespace privat
	{
		template<class T>
		struct HolderDereference
		{
			typedef T &type;
		};

		template<>
		struct HolderDereference<void>
		{
			typedef void type;
		};

		CAGE_CORE_API bool isHolderShareable(const Delegate<void(void *)> &deleter);
		CAGE_CORE_API void incHolderShareable(void *ptr, const Delegate<void(void *)> &deleter);
		CAGE_CORE_API void makeHolderShareable(void *&ptr, Delegate<void(void *)> &deleter);

		template<class T>
		struct HolderBase
		{
			HolderBase() {}
			explicit HolderBase(T *data, void *ptr, Delegate<void(void*)> deleter) : deleter_(deleter), ptr_(ptr), data_(data) {}

			HolderBase(const HolderBase &) = delete;
			HolderBase(HolderBase &&other) noexcept
			{
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
			}

			HolderBase &operator = (const HolderBase &) = delete;
			HolderBase &operator = (HolderBase &&other) noexcept
			{
				if (this == &other)
					return *this;
				if (deleter_)
					deleter_(ptr_);
				deleter_ = other.deleter_;
				ptr_ = other.ptr_;
				data_ = other.data_;
				other.deleter_.clear();
				other.ptr_ = nullptr;
				other.data_ = nullptr;
				return *this;
			}

			~HolderBase()
			{
				data_ = nullptr;
				void *tmpPtr = ptr_;
				Delegate<void(void *)> tmpDeleter = deleter_;
				ptr_ = nullptr;
				deleter_.clear();
				// calling the deleter is purposefully deferred to until this holder is cleared first
				if (tmpDeleter)
					tmpDeleter(tmpPtr);
			}

			explicit operator bool() const noexcept
			{
				return !!data_;
			}

			T *operator -> () const
			{
				CAGE_ASSERT(data_);
				return data_;
			}

			typename HolderDereference<T>::type operator * () const
			{
				CAGE_ASSERT(data_);
				return *data_;
			}

			T *get() const
			{
				return data_;
			}

			void clear()
			{
				if (deleter_)
					deleter_(ptr_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
			}

			bool isShareable() const
			{
				return isHolderShareable(deleter_);
			}

			Holder<T> share() const
			{
				incHolderShareable(ptr_, deleter_);
				return Holder<T>(data_, ptr_, deleter_);
			}

			Holder<T> makeShareable() &&
			{
				Holder<T> tmp(data_, ptr_, deleter_);
				makeHolderShareable(tmp.ptr_, tmp.deleter_);
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
				return tmp;
			}

		protected:
			void erase()
			{
				deleter_.clear();
				ptr_ = nullptr;
				data_ = nullptr;
			}

			Delegate<void(void *)> deleter_;
			void *ptr_ = nullptr; // pointer to deallocate
			T *data_ = nullptr; // pointer to the object (may differ in case of classes with inheritance)
		};
	}

	template<class T>
	struct Holder : public privat::HolderBase<T>
	{
		using privat::HolderBase<T>::HolderBase;

		template<class M>
		Holder<M> dynCast() &&
		{
			M *m = dynamic_cast<M*>(this->data_);
			if (!m && this->data_)
				CAGE_THROW_ERROR(Exception, "bad dynamic cast");
			Holder<M> tmp(m, this->ptr_, this->deleter_);
			this->erase();
			return tmp;
		}

		template<class M>
		Holder<M> cast() &&
		{
			Holder<M> tmp(static_cast<M*>(this->data_), this->ptr_, this->deleter_);
			this->erase();
			return tmp;
		}
	};

	// pointer range

	template<class T>
	struct PointerRange
	{
	private:
		T *begin_;
		T *end_;

	public:
		typedef uintPtr size_type;

		PointerRange() : begin_(nullptr), end_(nullptr) {}
		template<class U>
		PointerRange(U *begin, U *end) : begin_(begin), end_(end) {}
		template<class U>
		PointerRange(const PointerRange<U> &other) : begin_(other.begin()), end_(other.end()) {}
		template<class U>
		PointerRange(U &&other) : begin_(other.data()), end_(other.data() + other.size()) {}

		T *begin() const { return begin_; }
		T *end() const { return end_; }
		T *data() const { return begin_; }
		size_type size() const { return end_ - begin_; }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size()); return begin_[idx]; }
	};

	template<class T>
	struct Holder<PointerRange<T>> : public privat::HolderBase<PointerRange<T>>
	{
		using privat::HolderBase<PointerRange<T>>::HolderBase;
		typedef typename PointerRange<T>::size_type size_type;

		operator Holder<PointerRange<const T>> () &&
		{
			Holder<PointerRange<const T>> tmp((PointerRange<const T>*)this->data_, this->ptr_, this->deleter_);
			this->erase();
			return tmp;
		}

		T *begin() const { return this->data_->begin(); }
		T *end() const { return this->data_->end(); }
		T *data() const { return begin(); }
		size_type size() const { return end() - begin(); }
		bool empty() const { return size() == 0; }
		T &operator[] (uint32 idx) const { CAGE_ASSERT(idx < size()); return begin()[idx]; }
	};

	// memory arena

	namespace privat
	{
		struct CageNewTrait
		{};
	}
}

inline void *operator new(cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void *operator new[](cage::uintPtr size, void *ptr, cage::privat::CageNewTrait) noexcept
{
	return ptr;
}

inline void operator delete(void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}
inline void operator delete[](void *ptr, void *ptr2, cage::privat::CageNewTrait) noexcept {}

namespace cage
{
	struct CAGE_CORE_API MemoryArena
	{
	private:
		struct Stub
		{
			void *(*alloc)(void *, uintPtr, uintPtr);
			void(*dealloc)(void *, void *);
			void(*fls)(void *);

			template<class A>
			static void *allocate(void *inst, uintPtr size, uintPtr alignment)
			{
				return ((A*)inst)->allocate(size, alignment);
			}

			template<class A>
			static void deallocate(void *inst, void *ptr) noexcept
			{
				((A*)inst)->deallocate(ptr);
			}

			template<class A>
			static void flush(void *inst) noexcept
			{
				((A*)inst)->flush();
			}

			template<class A>
			static Stub init() noexcept
			{
				Stub s;
				s.alloc = &allocate<A>;
				s.dealloc = &deallocate<A>;
				s.fls = &flush<A>;
				return s;
			}
		} *stub = nullptr;
		void *inst = nullptr;

	public:
		MemoryArena() noexcept;

		template<class A>
		explicit MemoryArena(A *a) noexcept
		{
			static Stub stb = Stub::init<A>();
			this->stub = &stb;
			inst = a;
		}

		MemoryArena(const MemoryArena &) = default;
		MemoryArena(MemoryArena &&) = default;
		MemoryArena &operator = (const MemoryArena &) = default;
		MemoryArena &operator = (MemoryArena &&) = default;

		void *allocate(uintPtr size, uintPtr alignment);
		void deallocate(void *ptr) noexcept;
		void flush() noexcept;

		template<class T, class... Ts>
		T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new(ptr, privat::CageNewTrait()) T(templates::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		Holder<T> createHolder(Ts... vs)
		{
			Delegate<void(void*)> d;
			d.bind<MemoryArena, &MemoryArena::destroy<T>>(this);
			T *p = createObject<T>(templates::forward<Ts>(vs)...);
			return Holder<T>(p, p, d);
		};

		template<class Interface, class Impl, class... Ts>
		Holder<Interface> createImpl(Ts... vs)
		{
			return createHolder<Impl>(templates::forward<Ts>(vs)...).template cast<Interface>();
		};

		template<class T>
		void destroy(void *ptr) noexcept
		{
			if (!ptr)
				return;
			try
			{
				((T*)ptr)->~T();
			}
			catch (...)
			{
				privat::runtimeAssertFailure("exception thrown in destructor", __FILE__, __LINE__, __FUNCTION__);
			}
			deallocate(ptr);
		}

		bool operator == (const MemoryArena &other) const noexcept;
		bool operator != (const MemoryArena &other) const noexcept;
	};

	namespace detail
	{
		CAGE_CORE_API MemoryArena &systemArena();
	}

	// app time

	CAGE_CORE_API uint64 getApplicationTime();
}

#endif // guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
