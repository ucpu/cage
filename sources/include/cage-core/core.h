#ifndef guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
#define guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_

#include <cstdint>
#include <limits>
#include <utility>
#include <type_traits>

#if !defined(CAGE_DEBUG) && !defined(NDEBUG)
#define CAGE_DEBUG
#endif
#if defined(CAGE_DEBUG) && defined(NDEBUG)
#error CAGE_DEBUG and NDEBUG are incompatible
#endif

#if defined(_MSC_VER)
#define CAGE_API_EXPORT __declspec(dllexport)
#define CAGE_API_IMPORT __declspec(dllimport)
#define CAGE_API_PRIVATE
#define CAGE_FORCE_INLINE __forceinline
#else
#define CAGE_API_EXPORT [[gnu::visibility("default")]]
#define CAGE_API_IMPORT [[gnu::visibility("default")]]
#define CAGE_API_PRIVATE [[gnu::visibility("hidden")]]
#define CAGE_FORCE_INLINE __attribute__((always_inline)) inline
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

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif

#ifdef CAGE_DEBUG
#ifndef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT_ENABLED
#endif
#endif
#ifdef CAGE_ASSERT_ENABLED
#define CAGE_ASSERT(EXPR) { if (!(EXPR)) { ::cage::privat::runtimeAssertFailure(__FUNCTION__, __FILE__, __LINE__, #EXPR); int i_ = 42; } }
#else
#define CAGE_ASSERT(EXPR) {}
#endif

#define CAGE_THROW_SILENT(EXCEPTION, ...) { EXCEPTION e_(__FUNCTION__, __FILE__, __LINE__, ::cage::SeverityEnum::Error, __VA_ARGS__); throw e_; }
#define CAGE_THROW_ERROR(EXCEPTION, ...) { EXCEPTION e_(__FUNCTION__, __FILE__, __LINE__, ::cage::SeverityEnum::Error, __VA_ARGS__); e_.makeLog(); throw e_; }
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) { EXCEPTION e_(__FUNCTION__, __FILE__, __LINE__, ::cage::SeverityEnum::Critical, __VA_ARGS__); e_.makeLog(); throw e_; }

#define CAGE_LOG_THROW(MESSAGE) ::cage::privat::makeLogThrow(__FUNCTION__, __FILE__, __LINE__, MESSAGE)

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FUNCTION__, __FILE__, __LINE__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FUNCTION__, __FILE__, __LINE__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FUNCTION__, __FILE__, __LINE__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FUNCTION__, __FILE__, __LINE__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) {}
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) {}
#endif

namespace cage
{
	// numeric types

	using uint8 =  std::uint8_t;
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;
	using sint8 =  std::int8_t;
	using sint16 = std::int16_t;
	using sint32 = std::int32_t;
	using sint64 = std::int64_t;

#ifdef CAGE_ARCHITECTURE_64
	using uintPtr = uint64;
	using sintPtr = sint64;
#else
	using uintPtr = uint32;
	using sintPtr = sint32;
#endif

	// forward declarations

	namespace detail
	{
		template<uint32 N> struct StringBase;
		template<uint32 N> struct StringizerBase;
	}
	using string = detail::StringBase<995>;
	using stringizer = detail::StringizerBase<995>;

	template<class T> struct PointerRange;

	struct real;
	struct rads;
	struct degs;
	struct vec2;
	struct ivec2;
	struct vec3;
	struct ivec3;
	struct vec4;
	struct ivec4;
	struct quat;
	struct mat3;
	struct mat4;
	struct transform;

	struct Line;
	struct Triangle;
	struct Plane;
	struct Sphere;
	struct Aabb;
	struct Cone;
	struct Frustum;

	class AssetManager;
	struct AssetManagerCreateConfig;
	struct AssetContext;
	struct AssetScheme;
	struct AssetHeader;
	struct AssetHeader;
	enum class AudioFormatEnum : uint32;
	class Audio;
	class AudioChannelsConverter;
	struct AudioChannelsConverterCreateConfig;
	struct AudioDirectionalProcessConfig;
	class AudioDirectionalConverter;
	struct AudioDirectionalConverterCreateConfig;
	enum class StereoModeEnum : uint32;
	enum class StereoEyeEnum : uint32;
	struct StereoCameraInput;
	struct StereoCameraOutput;
	class Collider;
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
	class EntityManager;
	class EntityComponent;
	class Entity;
	class EntityGroup;
	struct FileMode;
	class File;
	enum class PathTypeFlags : uint32;
	class FilesystemWatcher;
	class DirectoryList;
	template<class Value, class Compare> struct FlatSet;
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
	class MarchingCubes;
	struct MarchingCubesCreateConfig;
	struct OutOfMemory;
	class VirtualMemory;
	struct MemoryBuffer;
	enum class MeshTypeEnum : uint32;
	struct MeshMergeCloseVerticesConfig;
	struct MeshSimplifyConfig;
	struct MeshRegularizeConfig;
	struct MeshUnwrapConfig;
	struct MeshGenerateTextureConfig;
	struct MeshGenerateNormalsConfig;
	struct MeshGenerateTangentsConfig;
	struct MeshExportObjConfig;
	class Mesh;
	struct DiscoveryPeer;
	class DiscoveryClient;
	class DiscoveryServer;
	struct GinnelStatistics;
	class GinnelConnection;
	class GinnelServer;
	class TcpConnection;
	class TcpServer;
	struct Disconnected;
	enum class NoiseTypeEnum : uint32;
	enum class NoiseInterpolationEnum : uint32;
	enum class NoiseFractalTypeEnum : uint32;
	enum class NoiseDistanceEnum : uint32;
	enum class NoiseOperationEnum : uint32;
	class NoiseFunction;
	struct NoiseFunctionCreateConfig;
	template<class T> struct PointerRangeHolder;
	class SampleRateConverter;
	struct SampleRateConverterCreateConfig;
	struct ProcessCreateConfig;
	class Process;
	struct RandomGenerator;
	class RectPacking;
	class SampleRateConverter;
	struct SampleRateConverterCreateConfig;
	enum class ScheduleTypeEnum : uint32;
	struct ScheduleCreateConfig;
	struct ScheduleStatistics;
	class Schedule;
	struct SchedulerCreateConfig;
	class Scheduler;
	template<class T> struct ScopeLock;
	struct Serializer;
	struct Deserializer;
	class SkeletalAnimation;
	class SkeletonRig;
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

	// Immovable

	struct CAGE_CORE_API Immovable
	{
		Immovable() = default;
		Immovable(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator = (const Immovable &) = delete;
		Immovable &operator = (Immovable &&) = delete;
	};

	struct CAGE_CORE_API Noncopyable
	{
		Noncopyable() = default;
		Noncopyable(const Noncopyable &) = delete;
		Noncopyable(Noncopyable &&) = default;
		Noncopyable &operator = (const Noncopyable &) = delete;
		Noncopyable &operator = (Noncopyable &&) = default;
	};

	// string literal

	struct StringLiteral
	{
		StringLiteral() = default;
		template<uint32 N> CAGE_FORCE_INLINE StringLiteral(const char(&str)[N]) noexcept : str(str) {}
		CAGE_FORCE_INLINE explicit StringLiteral(const char *str) noexcept : str(str) {}
		CAGE_FORCE_INLINE operator const char *() const noexcept { return str; }
	private:
		const char *str = nullptr;
	};

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
		CAGE_CORE_API uint64 makeLog(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral component, const string &message, bool continuous, bool debug) noexcept;
		CAGE_CORE_API void makeLogThrow(StringLiteral function, StringLiteral file, uint32 line, const string &message) noexcept;
		CAGE_CORE_API void runtimeAssertFailure(StringLiteral function, StringLiteral file, uint32 line, StringLiteral expt);
	}

	namespace detail
	{
		CAGE_CORE_API void logCurrentCaughtException();
	}

	// with CAGE_ASSERT_ENABLED numeric_cast validates that the value is in range of the target type, preventing overflows
	// without CAGE_ASSERT_ENABLED numeric_cast is the same as static_cast
	template<class To, class From>
	CAGE_FORCE_INLINE constexpr To numeric_cast(From from)
	{
		if constexpr (!std::is_floating_point_v<To> && std::is_floating_point_v<From>)
		{
			CAGE_ASSERT(from >= (From)std::numeric_limits<To>::min());
			CAGE_ASSERT(from <= (From)std::numeric_limits<To>::max());
		}
		else if constexpr (!std::is_floating_point_v<To> && !std::is_floating_point_v<From>)
		{
			if constexpr (std::numeric_limits<To>::is_signed && !std::numeric_limits<From>::is_signed)
			{
				CAGE_ASSERT(from <= static_cast<typename std::make_unsigned_t<To>>(std::numeric_limits<To>::max()));
			}
			else if constexpr (!std::numeric_limits<To>::is_signed && std::numeric_limits<From>::is_signed)
			{
				CAGE_ASSERT(from >= 0);
				CAGE_ASSERT(static_cast<typename std::make_unsigned_t<From>>(from) <= std::numeric_limits<To>::max());
			}
			else
			{
				CAGE_ASSERT(from >= std::numeric_limits<To>::lowest());
				CAGE_ASSERT(from <= std::numeric_limits<To>::max());
			}
		}
		return static_cast<To>(from);
	}

	// with CAGE_ASSERT_ENABLED class_cast verifies that dynamic_cast would succeed
	// without CAGE_ASSERT_ENABLED class_cast is the same as static_cast
	template<class To, class From>
	CAGE_FORCE_INLINE constexpr To class_cast(From from)
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
			CAGE_FORCE_INLINE constexpr operator T () const noexcept
			{
				return std::numeric_limits<T>::max();
			}
		};

		// todo replace with spaceship operator (c++20)
		template<class T> CAGE_FORCE_INLINE constexpr bool operator == (T lhs, MaxValue rhs) noexcept { return lhs == std::numeric_limits<T>::max(); }
		template<class T> CAGE_FORCE_INLINE constexpr bool operator != (T lhs, MaxValue rhs) noexcept { return lhs != std::numeric_limits<T>::max(); }
		template<class T> CAGE_FORCE_INLINE constexpr bool operator <= (T lhs, MaxValue rhs) noexcept { return lhs <= std::numeric_limits<T>::max(); }
		template<class T> CAGE_FORCE_INLINE constexpr bool operator >= (T lhs, MaxValue rhs) noexcept { return lhs >= std::numeric_limits<T>::max(); }
		template<class T> CAGE_FORCE_INLINE constexpr bool operator <  (T lhs, MaxValue rhs) noexcept { return lhs <  std::numeric_limits<T>::max(); }
		template<class T> CAGE_FORCE_INLINE constexpr bool operator >  (T lhs, MaxValue rhs) noexcept { return lhs >  std::numeric_limits<T>::max(); }
	}

	constexpr privat::MaxValue m = privat::MaxValue();

	// template magic

	namespace templates
	{
		// todo replace with spaceship operator (c++20)
		template<class T>
		struct Comparable
		{
			CAGE_FORCE_INLINE friend bool operator == (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) == 0; }
			CAGE_FORCE_INLINE friend bool operator != (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) != 0; }
			CAGE_FORCE_INLINE friend bool operator <= (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) <= 0; }
			CAGE_FORCE_INLINE friend bool operator >= (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) >= 0; }
			CAGE_FORCE_INLINE friend bool operator <  (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) <  0; }
			CAGE_FORCE_INLINE friend bool operator >  (const T &lhs, const T &rhs) noexcept { return compare(lhs, rhs) >  0; }
		};
	}

	// enum class bit operators

	template<class T> struct enable_bitmask_operators { static constexpr bool enable = false; };
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator ~ (T lhs) noexcept { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(lhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator | (T lhs, T rhs) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator & (T lhs, T rhs) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator ^ (T lhs, T rhs) noexcept { return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator |= (T &lhs, T rhs) noexcept { return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator &= (T &lhs, T rhs) noexcept { return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, T> operator ^= (T &lhs, T rhs) noexcept { return lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs)); }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, bool> any(T lhs) noexcept { return static_cast<std::underlying_type_t<T>>(lhs) != 0; }
	template<class T> CAGE_FORCE_INLINE constexpr typename std::enable_if_t<enable_bitmask_operators<T>::enable, bool> none(T lhs) noexcept { return static_cast<std::underlying_type_t<T>>(lhs) == 0; }

	// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(TYPE) template<> struct enable_bitmask_operators<TYPE> { static constexpr bool enable = true; };

	// exceptions

	struct CAGE_CORE_API Exception
	{
		explicit Exception(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral message) noexcept;
		virtual ~Exception() noexcept;

		void makeLog(); // check conditions and call log()
		virtual void log();

		StringLiteral function;
		StringLiteral file;
		StringLiteral message;
		uint32 line = 0;
		SeverityEnum severity = SeverityEnum::Critical;
	};

	struct CAGE_CORE_API NotImplemented : public Exception
	{
		using Exception::Exception;
		void log() override;
	};

	struct CAGE_CORE_API SystemError : public Exception
	{
		explicit SystemError(StringLiteral function, StringLiteral file, uint32 line, SeverityEnum severity, StringLiteral message, sint64 code) noexcept;
		void log() override;
		sint64 code = 0;
	};

	// string

	namespace privat
	{
#define GCHL_GENERATE(TYPE) \
		CAGE_CORE_API uint32 toString(char *s, uint32 n, TYPE value); \
		CAGE_CORE_API void fromString(const PointerRange<const char> &str, TYPE &value);
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
		CAGE_CORE_API uint32 toString(char *s, uint32 n, const char *src);
		CAGE_CORE_API int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl) noexcept;
	}

	namespace detail
	{
		CAGE_CORE_API void *memset(void *destination, int value, uintPtr num) noexcept;
		CAGE_CORE_API void *memcpy(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API void *memmove(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num) noexcept;

		template<uint32 N>
		struct StringizerBase;

		template<uint32 N>
		struct StringBase : templates::Comparable<StringBase<N>>
		{
			// constructors
			StringBase() noexcept = default;

			template<uint32 M>
			StringBase(const StringBase<M> &other) noexcept(N >= M)
			{
				if constexpr (N < M)
				{
					if (other.length() > N)
						CAGE_THROW_ERROR(Exception, "string truncation");
				}
				current = other.length();
				detail::memcpy(value, other.c_str(), current);
				value[current] = 0;
			}

			template<uint32 M>
			CAGE_FORCE_INLINE StringBase(const StringizerBase<M> &other);

			explicit StringBase(const PointerRange<const char> &range);

			CAGE_FORCE_INLINE explicit StringBase(char other) noexcept
			{
				value[0] = other;
				value[1] = 0;
				current = 1;
			}

			CAGE_FORCE_INLINE explicit StringBase(char *other)
			{
				current = privat::toString(value, N, other);
			}

			CAGE_FORCE_INLINE StringBase(const char *other)
			{
				current = privat::toString(value, N, other);
			}

			// compound operators
			StringBase &operator += (const StringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				detail::memcpy(value + current, other.value, other.current);
				current += other.current;
				value[current] = 0;
				return *this;
			}

			// binary operators
			CAGE_FORCE_INLINE StringBase operator + (const StringBase &other) const
			{
				return StringBase(*this) += other;
			}

			CAGE_FORCE_INLINE char &operator [] (uint32 idx)
			{
				CAGE_ASSERT(idx < current);
				return value[idx];
			}

			CAGE_FORCE_INLINE const char &operator [] (uint32 idx) const
			{
				CAGE_ASSERT(idx < current);
				return value[idx];
			}

			// methods
			CAGE_FORCE_INLINE const char *c_str() const
			{
				CAGE_ASSERT(value[current] == 0);
				return value;
			}

			CAGE_FORCE_INLINE const char *begin() const noexcept
			{
				return value;
			}

			CAGE_FORCE_INLINE const char *end() const noexcept
			{
				return value + current;
			}

			CAGE_FORCE_INLINE const char *data() const noexcept
			{
				return value;
			}

			CAGE_FORCE_INLINE uint32 size() const noexcept
			{
				return current;
			}

			CAGE_FORCE_INLINE uint32 length() const noexcept
			{
				return current;
			}

			CAGE_FORCE_INLINE bool empty() const noexcept
			{
				return current == 0;
			}

			CAGE_FORCE_INLINE char *rawData() noexcept
			{
				return value;
			}

			CAGE_FORCE_INLINE uint32 &rawLength() noexcept
			{
				return current;
			}

			static constexpr uint32 MaxLength = N;
			using value_type = char;

		private:
			char value[N + 1] = {};
			uint32 current = 0;
		};

		template<uint32 Na, uint32 Nb>
		CAGE_FORCE_INLINE int compare(const StringBase<Na> &a, const StringBase<Nb> &b) noexcept
		{
			return privat::stringComparison(a.c_str(), a.size(), b.c_str(), b.size());
		}

		template<uint32 N>
		struct StringizerBase
		{
			StringBase<N> value;

			template<uint32 M>
			CAGE_FORCE_INLINE StringizerBase<N> &operator + (const StringizerBase<M> &other)
			{
				value += other.value;
				return *this;
			}

			template<uint32 M>
			CAGE_FORCE_INLINE StringizerBase<N> &operator + (const StringBase<M> &other)
			{
				value += other;
				return *this;
			}

			CAGE_FORCE_INLINE StringizerBase<N> &operator + (const char *other)
			{
				value += other;
				return *this;
			}

			CAGE_FORCE_INLINE StringizerBase<N> &operator + (char *other)
			{
				value += other;
				return *this;
			}

			template<class T>
			CAGE_FORCE_INLINE StringizerBase<N> &operator + (T *other)
			{
				return *this + (uintPtr)other;
			}

#define GCHL_GENERATE(TYPE) \
			CAGE_FORCE_INLINE StringizerBase<N> &operator + (TYPE other) \
			{ \
				StringBase<30> tmp; \
				tmp.rawLength() = privat::toString(tmp.rawData(), 30, other); \
				return *this + tmp; \
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

			// allow to use l-value-reference operator overloads with r-value-reference stringizer
			template<class T>
			CAGE_FORCE_INLINE StringizerBase<N> &operator + (const T &other) &&
			{
				return *this + other;
			}
		};

		template<uint32 N> template<uint32 M>
		CAGE_FORCE_INLINE StringBase<N>::StringBase(const StringizerBase<M> &other) : StringBase(other.value)
		{}
	}

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
		constexpr Delegate &bind() noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return F(std::forward<Ts>(vs)...);
			};
			return *this;
		}

		template<class D, R(*F)(D, Ts...)>
		Delegate &bind(D d) noexcept
		{
			static_assert(sizeof(d) <= sizeof(inst));
			static_assert(std::is_trivially_copyable_v<D> && std::is_trivially_destructible_v<D>);
			union U
			{
				void *p;
				D d;
			};
			fnc = +[](void *inst, Ts... vs) {
				U u;
				u.p = inst;
				return F(u.d, std::forward<Ts>(vs)...);
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
				return (((C*)inst)->*F)(std::forward<Ts>(vs)...);
			};
			inst = i;
			return *this;
		}

		template<class C, R(C::*F)(Ts...) const>
		Delegate &bind(const C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((const C*)inst)->*F)(std::forward<Ts>(vs)...);
			};
			inst = const_cast<C*>(i);
			return *this;
		}

		CAGE_FORCE_INLINE constexpr explicit operator bool() const noexcept
		{
			return !!fnc;
		}

		CAGE_FORCE_INLINE constexpr void clear() noexcept
		{
			inst = nullptr;
			fnc = nullptr;
		}

		CAGE_FORCE_INLINE constexpr R operator ()(Ts... vs) const
		{
			CAGE_ASSERT(!!*this);
			return fnc(inst, std::forward<Ts>(vs)...);
		}

		CAGE_FORCE_INLINE constexpr bool operator == (const Delegate &other) const noexcept
		{
			return fnc == other.fnc && inst == other.inst;
		}

		CAGE_FORCE_INLINE constexpr bool operator != (const Delegate &other) const noexcept
		{
			return !(*this == other);
		}
	};

	// holder

	template<class T> struct Holder;

	namespace privat
	{
		struct CAGE_CORE_API HolderControlBase : private Immovable
		{
			Delegate<void(void *)> deleter;
			void *deletee = nullptr;
			void inc();
			void dec();
		private:
			alignas(4) volatile uint32 counter = 0;
		};

		template<class T>
		struct HolderBase : private Noncopyable
		{
			HolderBase() noexcept = default;
			explicit HolderBase(T *data, HolderControlBase *control) noexcept : data_(data), control_(control)
			{
				if (control_)
					control_->inc();
			}

			template<class U>
			HolderBase(T *data, HolderBase<U> &&base) noexcept : data_(data), control_(base.control_)
			{
				base.data_ = nullptr;
				base.control_ = nullptr;
			}

			HolderBase(HolderBase &&other) noexcept : HolderBase(other.data_, std::move(other))
			{}

			HolderBase &operator = (HolderBase &&other) noexcept
			{
				if (this == &other)
					return *this;
				if (control_)
					control_->dec();
				data_ = other.data_;
				control_ = other.control_;
				other.data_ = nullptr;
				other.control_ = nullptr;
				return *this;
			}

			~HolderBase()
			{
				clear();
			}

			CAGE_FORCE_INLINE explicit operator bool() const noexcept
			{
				return !!data_;
			}

			CAGE_FORCE_INLINE T *operator -> () const
			{
				CAGE_ASSERT(data_);
				return data_;
			}

			template<bool Enable = !std::is_same_v<T, void>>
			CAGE_FORCE_INLINE typename std::enable_if_t<Enable, T> &operator * () const
			{
				CAGE_ASSERT(data_);
				return *data_;
			}

			CAGE_FORCE_INLINE T *get() const noexcept
			{
				return data_;
			}

			CAGE_FORCE_INLINE T *operator + () const noexcept
			{
				return data_;
			}

			void clear()
			{
				data_ = nullptr;
				HolderControlBase *tmpCtrl = control_;
				control_ = nullptr;
				// decrementing the counter is purposefully deferred to until this holder is cleared first
				if (tmpCtrl)
					tmpCtrl->dec();
			}

			CAGE_FORCE_INLINE Holder<T> share() const
			{
				return Holder<T>(data_, control_);
			}

		protected:
			T *data_ = nullptr;
			HolderControlBase *control_ = nullptr;

			template<class U>
			friend struct HolderBase;
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
			return Holder<M>(m, std::move(*this));
		}

		template<class M>
		Holder<M> cast() &&
		{
			return Holder<M>(static_cast<M*>(this->data_), std::move(*this));
		}

		CAGE_FORCE_INLINE operator Holder<const T>() &&
		{
			return Holder<const T>((const T*)this->data_, std::move(*this));
		}
	};

	// pointer range

	namespace privat
	{
		template<class K> struct TerminalZero { static constexpr int value = 0; };
		template<> struct TerminalZero<char> { static constexpr int value = -1; };
	}

	template<class T>
	struct PointerRange
	{
	private:
		T *begin_ = nullptr;
		T *end_ = nullptr;

	public:
		using size_type = uintPtr;
		using value_type = T;

		constexpr PointerRange() noexcept = default;
		constexpr PointerRange(const PointerRange<T> &other) noexcept = default;
		CAGE_FORCE_INLINE constexpr PointerRange(T *begin, T *end) noexcept : begin_(begin), end_(end) {}
		template<uint32 N>
		CAGE_FORCE_INLINE constexpr PointerRange(T (&arr)[N]) noexcept : begin_(arr), end_(arr + N + privat::TerminalZero<std::remove_cv_t<T>>::value) {}
		template<class U, std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<typename U::value_type>>, int> = 0>
		CAGE_FORCE_INLINE constexpr PointerRange(U &other) : begin_(other.data()), end_(other.data() + other.size()) {}
		template<class U, std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<typename U::value_type>>, int> = 0>
		CAGE_FORCE_INLINE constexpr PointerRange(U &&other) : begin_(other.data()), end_(other.data() + other.size()) {}

		CAGE_FORCE_INLINE constexpr T *begin() const noexcept { return begin_; }
		CAGE_FORCE_INLINE constexpr T *end() const noexcept { return end_; }
		CAGE_FORCE_INLINE constexpr T *data() const noexcept { return begin_; }
		CAGE_FORCE_INLINE constexpr size_type size() const noexcept { return end_ - begin_; }
		CAGE_FORCE_INLINE constexpr bool empty() const noexcept { return size() == 0; }
		CAGE_FORCE_INLINE constexpr T &operator[] (size_type idx) const { CAGE_ASSERT(idx < size()); return begin_[idx]; }
	};

	template<class T>
	struct Holder<PointerRange<T>> : public privat::HolderBase<PointerRange<T>>
	{
		using privat::HolderBase<PointerRange<T>>::HolderBase;
		using size_type = typename PointerRange<T>::size_type;
		using value_type = typename PointerRange<T>::value_type;

		CAGE_FORCE_INLINE operator Holder<PointerRange<const T>> () &&
		{
			return Holder<PointerRange<const T>>((PointerRange<const T>*)this->data_, std::move(*this));
		}

		CAGE_FORCE_INLINE T *begin() const noexcept { return this->data_ ? this->data_->begin() : nullptr; }
		CAGE_FORCE_INLINE T *end() const noexcept { return this->data_ ? this->data_->end() : nullptr; }
		CAGE_FORCE_INLINE T *data() const noexcept { return begin(); }
		CAGE_FORCE_INLINE size_type size() const noexcept { return end() - begin(); }
		CAGE_FORCE_INLINE bool empty() const noexcept { return size() == 0; }
		CAGE_FORCE_INLINE T &operator[] (size_type idx) const { CAGE_ASSERT(idx < size()); return begin()[idx]; }
	};

	namespace detail
	{
		template<uint32 N>
		StringBase<N>::StringBase(const PointerRange<const char> &range)
		{
			if (range.size() > N)
				CAGE_THROW_ERROR(Exception, "string truncation");
			current = numeric_cast<uint32>(range.size());
			detail::memcpy(value, range.data(), range.size());
			value[current] = 0;
		}
	}

	// memory arena

	namespace privat
	{
		struct OperatorNewTrait {};
		struct MemoryArenaStub;
	}
}

CAGE_FORCE_INLINE void *operator new(cage::uintPtr size, void *ptr, cage::privat::OperatorNewTrait) noexcept { return ptr; }
CAGE_FORCE_INLINE void *operator new[](cage::uintPtr size, void *ptr, cage::privat::OperatorNewTrait) noexcept { return ptr; }
CAGE_FORCE_INLINE void operator delete(void *ptr, void *ptr2, cage::privat::OperatorNewTrait) noexcept {}
CAGE_FORCE_INLINE void operator delete[](void *ptr, void *ptr2, cage::privat::OperatorNewTrait) noexcept {}

namespace cage
{
	struct CAGE_CORE_API MemoryArena
	{
	private:
		privat::MemoryArenaStub *stub = nullptr;
		void *inst = nullptr;

	public:
		template<class A>
		inline explicit MemoryArena(A *a) noexcept;

		void *allocate(uintPtr size, uintPtr alignment);
		void deallocate(void *ptr);
		void flush();

		template<class T, class... Ts>
		CAGE_FORCE_INLINE T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new(ptr, privat::OperatorNewTrait()) T(std::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		CAGE_FORCE_INLINE Holder<T> createHolder(Ts... vs)
		{
			struct Ctrl : public privat::HolderControlBase
			{
				Ctrl(Ts... vs) : data(std::forward<Ts>(vs)...) {}
				T data;
			};
			Ctrl *p = createObject<Ctrl>(std::forward<Ts>(vs)...);
			p->deletee = p;
			p->deleter.template bind<MemoryArena, &MemoryArena::destroy<Ctrl>>(this);
			return Holder<T>(&p->data, p);
		};

		template<class Interface, class Impl, class... Ts>
		CAGE_FORCE_INLINE Holder<Interface> createImpl(Ts... vs)
		{
			return createHolder<Impl>(std::forward<Ts>(vs)...).template cast<Interface>();
		};

		template<class T>
		CAGE_FORCE_INLINE void destroy(void *ptr)
		{
			if (!ptr)
				return;
			try
			{
				((T *)ptr)->~T();
			}
			catch (...)
			{
				detail::logCurrentCaughtException();
				CAGE_ASSERT(!"exception thrown in destructor");
			}
			deallocate(ptr);
		}

		bool operator == (const MemoryArena &other) const noexcept;
		bool operator != (const MemoryArena &other) const noexcept;
	};

	CAGE_CORE_API MemoryArena &systemMemory();
	CAGE_CORE_API uint64 applicationTime();
}

#endif // guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
