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

	// numeric limits

	namespace privat
	{
		template<bool>
		struct helper_is_char_signed
		{};

		template<>
		struct helper_is_char_signed<true>
		{
			typedef signed char type;
		};

		template<>
		struct helper_is_char_signed<false>
		{
			typedef unsigned char type;
		};
	}

	namespace detail
	{
		template<class T>
		struct numeric_limits
		{
			static const bool is_specialized = false;
		};

		template<>
		struct numeric_limits<unsigned char>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint8 min() { return 0; };
			static constexpr const uint8 max() { return 255u; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint16 min() { return 0; };
			static constexpr const uint16 max() { return 65535u; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint32 min() { return 0; };
			static constexpr const uint32 max() { return 4294967295u; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<unsigned long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = false;
			static constexpr const uint64 min() { return 0; };
			static constexpr const uint64 max() { return 18446744073709551615LLu; };
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
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint8 min() { return -127 - 1; };
			static constexpr const sint8 max() { return  127; };
			typedef uint8 make_unsigned;
		};

		template<>
		struct numeric_limits<signed short>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint16 min() { return -32767 - 1; };
			static constexpr const sint16 max() { return  32767; };
			typedef uint16 make_unsigned;
		};

		template<>
		struct numeric_limits<signed int>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint32 min() { return -2147483647 - 1; };
			static constexpr const sint32 max() { return  2147483647; };
			typedef uint32 make_unsigned;
		};

		template<>
		struct numeric_limits<signed long long>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const sint64 min() { return -9223372036854775807LL - 1; };
			static constexpr const sint64 max() { return  9223372036854775807LL; };
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
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const float min() { return -1e+37f; };
			static constexpr const float max() { return  1e+37f; };
			typedef float make_unsigned;
		};

		template<>
		struct numeric_limits<double>
		{
			static const bool is_specialized = true;
			static const bool is_signed = true;
			static constexpr const double min() { return -1e+308; };
			static constexpr const double max() { return  1e+308; };
			typedef double make_unsigned;
		};

		// char is distinct type from both unsigned char and signed char
		template<>
		struct numeric_limits<char> : public numeric_limits<privat::helper_is_char_signed<(char)-1 < (char)0>::type>
		{};
	}

	// max struct

	namespace privat
	{
		// represents the maximum positive value possible in any numeric type
		struct MaxValue
		{
			template<class T>
			constexpr operator T () const
			{
				return detail::numeric_limits<T>::max();
			}
		};

		template<class T> constexpr bool operator == (T lhs, MaxValue rhs) { return lhs == detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator != (T lhs, MaxValue rhs) { return lhs != detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator <= (T lhs, MaxValue rhs) { return lhs <= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator >= (T lhs, MaxValue rhs) { return lhs >= detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator < (T lhs, MaxValue rhs) { return lhs < detail::numeric_limits<T>::max(); }
		template<class T> constexpr bool operator > (T lhs, MaxValue rhs) { return lhs > detail::numeric_limits<T>::max(); }
	}

	static constexpr const privat::MaxValue m = privat::MaxValue();

	// template magic

	namespace templates
	{
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
		template<typename T> struct is_lvalue_reference { static constexpr const bool value = false; };
		template<typename T> struct is_lvalue_reference<T&> { static constexpr const bool value = true; };
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type  &v) noexcept { return static_cast<T&&>(v); }
		template<class T> inline constexpr T &&forward(typename remove_reference<T>::type &&v) noexcept { static_assert(!is_lvalue_reference<T>::value, "invalid rvalue to lvalue conversion"); return static_cast<T&&>(v); }
		template<class T> inline constexpr typename remove_reference<T>::type &&move(T &&v) noexcept { return static_cast<typename remove_reference<T>::type&&>(v); }
	}

	// enum class bit operators

	template<class T> struct enable_bitmask_operators { static const bool enable = false; };
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ~ (T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(~static_cast<underlying>(lhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator | (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator & (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline constexpr typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^ (T lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator |= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator &= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, T>::type operator ^= (T &lhs, T rhs) { typedef typename templates::underlying_type<T>::type underlying; return lhs = static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)); }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type any(T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) != 0; }
	template<class T> inline typename templates::enable_if<enable_bitmask_operators<T>::enable, bool>::type none(T lhs) { typedef typename templates::underlying_type<T>::type underlying; return static_cast<underlying>(lhs) == 0; }

	// this macro has to be used inside namespace cage
#define GCHL_ENUM_BITS(TYPE) template<> struct enable_bitmask_operators<TYPE> { static const bool enable = true; };

	// Immovable

	struct CAGE_API Immovable
	{
		Immovable() = default;
		Immovable(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator = (const Immovable &) = delete;
		Immovable &operator = (Immovable &&) = delete;
	};

	// forward declarations

	enum class AssetStateEnum : uint32;
	class AssetManager;
	struct AssetManagerCreateConfig;
	struct AssetContext;
	struct AssetScheme;
	struct AssetHeader;
	struct AssetHeader;
	class CollisionMesh;
	struct CollisionPair;
	class CollisionQuery;
	class CollisionData;
	struct CollisionDataCreateConfig;
	class Mutex;
	class Barrier;
	class Semaphore;
	class ConditionalVariableBase;
	class ConditionalVariable;
	class Thread;
	struct ConcurrentQueueCreateConfig;
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
	class Ini;
	namespace detail { template<uint32 N> struct StringBase; }
	typedef detail::StringBase<1000> string;
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
	class Filesystem;
	struct line;
	struct triangle;
	struct plane;
	struct sphere;
	struct aabb;
	struct HashString;
	template<uint32 N> struct Guid;
	class Image;
	class LineReader;
	class Logger;
	class LoggerOutputFile;
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
	template<class T> struct MemoryArenaStd;
	struct MemoryBuffer;
	class TcpConnection;
	class TcpServer;
	struct UdpStatistics;
	class UdpConnection;
	class UdpServer;
	struct DiscoveryPeer;
	class DiscoveryClient;
	class DiscoveryServer;
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
	class SpatialData;
	struct SpatialDataCreateConfig;
	class BufferIStream;
	class BufferOStream;
	class SwapBufferGuard;
	struct SwapBufferGuardCreateConfig;
	class TextPack;
	class ThreadPool;
	class Timer;
	struct InvalidUtfString;
	enum class StereoModeEnum : uint32;
	enum class StereoEyeEnum : uint32;
	struct StereoCameraInput;
	struct StereoCameraOutput;
	template<class T, uint32 N = 16> struct VariableSmoothingBuffer;
}
