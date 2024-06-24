#ifndef guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
#define guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_

#include <compare>
#include <cstdint>
#include <limits>
#include <source_location>
#include <type_traits>
#include <utility>

#if defined(_MSC_VER)
	#define CAGE_ASSUME_TRUE(EXPR) __assume((bool)(EXPR))
#elif defined(__clang__)
	#define CAGE_ASSUME_TRUE(EXPR) __builtin_assume((bool)(EXPR))
#else
	#define CAGE_ASSUME_TRUE(EXPR)
#endif

#ifdef CAGE_ASSERT_ENABLED
	#define CAGE_ASSERT(EXPR) \
		{ \
			if (!(EXPR)) \
			{ \
				::cage::privat::runtimeAssertFailure(::std::source_location::current(), #EXPR); \
				int i_ = 42; \
				(void)i_; \
			} \
			CAGE_ASSUME_TRUE(EXPR); \
		}
#else
	#define CAGE_ASSERT(EXPR) \
		{ \
			CAGE_ASSUME_TRUE(EXPR); \
		}
#endif

#define CAGE_THROW_SILENT(EXCEPTION, ...) \
	{ \
		throw EXCEPTION(::std::source_location::current(), ::cage::SeverityEnum::Error, __VA_ARGS__); \
	}
#define CAGE_THROW_ERROR(EXCEPTION, ...) \
	{ \
		try \
		{ \
			throw EXCEPTION(::std::source_location::current(), ::cage::SeverityEnum::Error, __VA_ARGS__); \
		} \
		catch (const cage::Exception &e) \
		{ \
			e.makeLog(); \
			throw; \
		} \
	}
#define CAGE_THROW_CRITICAL(EXCEPTION, ...) \
	{ \
		try \
		{ \
			throw EXCEPTION(::std::source_location::current(), ::cage::SeverityEnum::Critical, __VA_ARGS__); \
		} \
		catch (const cage::Exception &e) \
		{ \
			e.makeLog(); \
			throw; \
		} \
	}

#define CAGE_LOG_THROW(MESSAGE) ::cage::privat::makeLogThrow(::std::source_location::current(), MESSAGE)

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(::std::source_location::current(), SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(::std::source_location::current(), SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
	#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(::std::source_location::current(), SEVERITY, COMPONENT, MESSAGE, false, true)
	#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(::std::source_location::current(), SEVERITY, COMPONENT, MESSAGE, true, true)
#else
	#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) \
		{}
	#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) \
		{}
#endif

namespace cage
{
	// numeric types

	using uint8 = std::uint8_t;
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;
	using sint8 = std::int8_t;
	using sint16 = std::int16_t;
	using sint32 = std::int32_t;
	using sint64 = std::int64_t;
	using uintPtr = std::conditional_t<sizeof(void *) == 8, std::uint64_t, std::uint32_t>;
	using sintPtr = std::conditional_t<sizeof(void *) == 8, std::int64_t, std::int32_t>;

	// forward declarations

	namespace detail
	{
		template<uint32 N>
		struct StringBase;
		template<uint32 N>
		struct StringizerBase;
	}
	using String = detail::StringBase<1019>;
	using Stringizer = detail::StringizerBase<1019>;

	struct Real;
	struct Rads;
	struct Degs;
	struct Vec2;
	struct Vec2i;
	struct Vec3;
	struct Vec3i;
	struct Vec4;
	struct Vec4i;
	struct Quat;
	struct Mat3;
	struct Mat4;
	struct Transform;
	struct Mat3x4;

	struct Line;
	struct Triangle;
	struct Plane;
	struct Sphere;
	struct Aabb;
	struct Cone;
	struct Frustum;

	class AssetManager;
	struct AssetScheme;

	// Immovable

	struct CAGE_CORE_API Immovable
	{
		Immovable() = default;
		Immovable(const Immovable &) = delete;
		Immovable(Immovable &&) = delete;
		Immovable &operator=(const Immovable &) = delete;
		Immovable &operator=(Immovable &&) = delete;
	};

	struct CAGE_CORE_API Noncopyable
	{
		Noncopyable() = default;
		Noncopyable(const Noncopyable &) = delete;
		Noncopyable(Noncopyable &&) = default;
		Noncopyable &operator=(const Noncopyable &) = delete;
		Noncopyable &operator=(Noncopyable &&) = default;
	};

	// string pointer

	struct StringPointer
	{
		constexpr StringPointer() = default;
		template<uint32 N>
		CAGE_FORCE_INLINE constexpr StringPointer(const char (&str)[N]) noexcept : str(str)
		{}
		CAGE_FORCE_INLINE constexpr explicit StringPointer(const char *str) noexcept : str(str) {}
		CAGE_FORCE_INLINE constexpr operator const char *() const noexcept { return str; }

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
		CAGE_CORE_API uint64 makeLog(const std::source_location &location, SeverityEnum severity, StringPointer component, const String &message, bool continuous, bool debug) noexcept;
		CAGE_CORE_API void makeLogThrow(const std::source_location &location, const String &message) noexcept;
		[[noreturn]] CAGE_CORE_API void runtimeAssertFailure(const std::source_location &location, StringPointer expt);
	}

	namespace detail
	{
		CAGE_CORE_API void logCurrentCaughtException() noexcept;
		[[noreturn]] CAGE_CORE_API void irrecoverableError(StringPointer explanation) noexcept; // eg exception in destructor, terminates the program
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
			CAGE_FORCE_INLINE consteval operator T() const noexcept
			{
				return std::numeric_limits<T>::max();
			}

			template<class T>
			CAGE_FORCE_INLINE constexpr auto operator<=>(T rhs) const noexcept
			{
				return std::numeric_limits<T>::max() <=> rhs;
			}

			template<class T>
			CAGE_FORCE_INLINE constexpr bool operator==(T rhs) const noexcept
			{
				return std::numeric_limits<T>::max() == rhs;
			}
		};
	}

	constexpr privat::MaxValue m = {};

	// exceptions

	struct CAGE_CORE_API Exception
	{
		explicit Exception(const std::source_location &location, SeverityEnum severity, StringPointer message) noexcept;
		virtual ~Exception() noexcept;

		void makeLog() const; // check conditions and call log()
		virtual void log() const;

		std::source_location location;
		StringPointer message;
		SeverityEnum severity = SeverityEnum::Critical;
	};

	struct CAGE_CORE_API SystemError : public Exception
	{
		explicit SystemError(const std::source_location &location, SeverityEnum severity, StringPointer message, sint64 code) noexcept;
		void log() const override;
		sint64 code = 0;
	};

	// array size

	template<class T, uintPtr N>
	constexpr uintPtr array_size(const T (&)[N]) noexcept
	{
		return N;
	}

	// pointer range

	namespace privat
	{
		// this can be moved directly into the PointerRange once compilers implemented CWG 727
		template<class K>
		struct TerminalZero
		{
			static constexpr int value = 0;
		};
		template<>
		struct TerminalZero<char>
		{
			static constexpr int value = -1;
		};
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
		CAGE_FORCE_INLINE constexpr PointerRange(T (&arr)[N]) noexcept : begin_(arr), end_(arr + N + privat::TerminalZero<std::remove_cv_t<T>>::value)
		{}
		template<class U>
		requires(std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<typename U::value_type>>)
		CAGE_FORCE_INLINE constexpr PointerRange(U &other) : begin_(other.data()), end_(other.data() + other.size())
		{}
		template<class U>
		requires(std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<typename U::value_type>>)
		CAGE_FORCE_INLINE constexpr PointerRange(U &&other) : begin_(other.data()), end_(other.data() + other.size())
		{}

		CAGE_FORCE_INLINE constexpr T *begin() const noexcept { return begin_; }
		CAGE_FORCE_INLINE constexpr T *end() const noexcept { return end_; }
		CAGE_FORCE_INLINE constexpr T *data() const noexcept { return begin_; }
		CAGE_FORCE_INLINE constexpr size_type size() const noexcept { return end_ - begin_; }
		CAGE_FORCE_INLINE constexpr bool empty() const noexcept { return size() == 0; }
		CAGE_FORCE_INLINE constexpr T &operator[](size_type idx) const
		{
			CAGE_ASSERT(idx < size());
			return begin_[idx];
		}

		template<class M>
		CAGE_FORCE_INLINE constexpr PointerRange<M> cast() const noexcept
		{
			static_assert(sizeof(T) == sizeof(M));
			static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_copyable_v<M>);
			return PointerRange<M>((M *)begin_, (M *)end_);
		}
	};

	// string

	namespace detail
	{
		CAGE_CORE_API void *memset(void *destination, int value, uintPtr num) noexcept;
		CAGE_CORE_API void *memcpy(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API void *memmove(void *destination, const void *source, uintPtr num) noexcept;
		CAGE_CORE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num) noexcept;

		CAGE_FORCE_INLINE constexpr char *memset(char *destination, int value, uintPtr num) noexcept
		{
			CAGE_ASSERT(destination);
			if (std::is_constant_evaluated())
			{
				char *res = destination;
				for (uintPtr i = 0; i < num; i++)
					*destination++ = (char)value;
				return res;
			}
			else
				return (char *)memset((void *)destination, value, num);
		}

		CAGE_FORCE_INLINE constexpr char *memcpy(char *destination, const char *source, uintPtr num) noexcept
		{
			CAGE_ASSERT(destination && source);
			if (std::is_constant_evaluated())
			{
				char *res = destination;
				for (uintPtr i = 0; i < num; i++)
					*destination++ = *source++;
				return res;
			}
			else
				return (char *)memcpy((void *)destination, (const void *)source, num);
		}

		CAGE_FORCE_INLINE constexpr char *memmove(char *destination, const char *source, uintPtr num) noexcept
		{
			CAGE_ASSERT(destination && source);
			const bool overlap = destination <= source + num && source <= destination + num;
			if (std::is_constant_evaluated() && !overlap)
				return memcpy(destination, source, num);
			else
				return (char *)memmove((void *)destination, (const void *)source, num);
		}

		CAGE_FORCE_INLINE constexpr int memcmp(const char *ptr1, const char *ptr2, uintPtr num) noexcept
		{
			CAGE_ASSERT(ptr1 && ptr2);
			if (std::is_constant_evaluated())
			{
				for (uintPtr i = 0; i < num; i++)
				{
					const int d = int(*ptr1++) - int(*ptr2++);
					if (d != 0)
						return d;
				}
				return 0;
			}
			else
				return memcmp((const void *)ptr1, (const void *)ptr2, num);
		}
	}

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
		CAGE_CORE_API uint32 toStringImpl(char *s, uint32 n, const char *src);
		CAGE_CORE_API int stringComparisonImpl(const char *ad, uint32 al, const char *bd, uint32 bl) noexcept;

		CAGE_FORCE_INLINE constexpr uint32 toString(char *s, uint32 n, const char *src)
		{
			CAGE_ASSERT(src);
			if (std::is_constant_evaluated())
			{
				uintPtr l = 0;
				while (src[l])
					l++;
				if (l > n)
					throw;
				if (l)
					detail::memcpy(s, src, l);
				s[l] = 0;
				return numeric_cast<uint32>(l);
			}
			else
				return toStringImpl(s, n, src);
		}

		CAGE_FORCE_INLINE constexpr int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl) noexcept
		{
			CAGE_ASSERT(ad && bd);
			if (std::is_constant_evaluated())
			{
				const uint32 l = al < bl ? al : bl;
				const int c = detail::memcmp(ad, bd, l);
				if (c == 0)
					return al == bl ? 0 : al < bl ? -1 : 1;
				return c;
			}
			else
				return stringComparisonImpl(ad, al, bd, bl);
		}
	}

	namespace detail
	{
		template<uint32 N>
		struct StringizerBase;

		template<uint32 N>
		struct alignas(32) StringBase
		{
			// constructors
			StringBase() noexcept = default;

			template<uint32 M>
			constexpr StringBase(const StringBase<M> &other) noexcept(N >= M)
			{
				if constexpr (N < M)
				{
					if (other.length() > N)
						CAGE_THROW_ERROR(Exception, "string truncation");
				}
				current = other.length();
				if (current)
					detail::memcpy(value, other.c_str(), current);
				value[current] = 0;
			}

			template<uint32 M>
			CAGE_FORCE_INLINE constexpr StringBase(const StringizerBase<M> &other);

			explicit constexpr StringBase(PointerRange<const char> range)
			{
				if (range.size() > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				current = numeric_cast<uint32>(range.size());
				if (!range.empty())
					detail::memcpy(value, range.data(), range.size());
				value[current] = 0;
			}

			CAGE_FORCE_INLINE explicit constexpr StringBase(char other) noexcept
			{
				value[0] = other;
				value[1] = 0;
				current = 1;
			}

			CAGE_FORCE_INLINE explicit constexpr StringBase(char *other) { current = privat::toString(value, N, other); }

			CAGE_FORCE_INLINE constexpr StringBase(const char *other) { current = privat::toString(value, N, other); }

			// compound operators
			constexpr StringBase &operator+=(const StringBase &other)
			{
				if (current + other.current > N)
					CAGE_THROW_ERROR(Exception, "string truncation");
				if (other.current)
					detail::memcpy(value + current, other.value, other.current);
				current += other.current;
				value[current] = 0;
				return *this;
			}

			// binary operators
			CAGE_FORCE_INLINE constexpr StringBase operator+(const StringBase &other) const { return StringBase(*this) += other; }

			CAGE_FORCE_INLINE constexpr char &operator[](uint32 idx)
			{
				CAGE_ASSERT(idx < current);
				return value[idx];
			}

			CAGE_FORCE_INLINE constexpr const char &operator[](uint32 idx) const
			{
				CAGE_ASSERT(idx < current);
				return value[idx];
			}

			CAGE_FORCE_INLINE constexpr auto operator<=>(const StringBase &other) const noexcept { return privat::stringComparison(c_str(), size(), other.c_str(), other.size()) <=> 0; }
			CAGE_FORCE_INLINE constexpr bool operator==(const StringBase &other) const noexcept { return privat::stringComparison(c_str(), size(), other.c_str(), other.size()) == 0; }

			// methods
			CAGE_FORCE_INLINE constexpr const char *c_str() const
			{
				CAGE_ASSERT(value[current] == 0);
				return value;
			}

			CAGE_FORCE_INLINE constexpr const char *begin() const noexcept { return value; }
			CAGE_FORCE_INLINE constexpr const char *end() const noexcept { return value + current; }
			CAGE_FORCE_INLINE constexpr const char *data() const noexcept { return value; }
			CAGE_FORCE_INLINE constexpr uint32 size() const noexcept { return current; }
			CAGE_FORCE_INLINE constexpr uint32 length() const noexcept { return current; }
			CAGE_FORCE_INLINE constexpr bool empty() const noexcept { return current == 0; }
			CAGE_FORCE_INLINE constexpr char *rawData() noexcept { return value; }
			CAGE_FORCE_INLINE constexpr uint32 &rawLength() noexcept { return current; }

			static constexpr uint32 MaxLength = N;
			using value_type = char;

		private:
			char value[N + 1] = {};
			uint32 current = 0;
		};

		template<uint32 N>
		struct alignas(32) StringizerBase
		{
			StringBase<N> value;

			template<uint32 M>
			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(const StringizerBase<M> &other)
			{
				value += other.value;
				return *this;
			}

			template<uint32 M>
			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(const StringBase<M> &other)
			{
				value += other;
				return *this;
			}

			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(const char *other)
			{
				value += other;
				return *this;
			}

			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(char *other)
			{
				value += other;
				return *this;
			}

			template<class T>
			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(T *other)
			{
				return *this + (uintPtr)other;
			}

#define GCHL_GENERATE(TYPE, SIZE) \
	CAGE_FORCE_INLINE StringizerBase<N> &operator+(TYPE other) \
	{ \
		StringBase<SIZE> tmp; \
		tmp.rawLength() = privat::toString(tmp.rawData(), SIZE, other); \
		return *this + tmp; \
	}
			GCHL_GENERATE(char, 4);
			GCHL_GENERATE(sint8, 4);
			GCHL_GENERATE(sint16, 6);
			GCHL_GENERATE(sint32, 11);
			GCHL_GENERATE(sint64, 21);
			GCHL_GENERATE(uint8, 3);
			GCHL_GENERATE(uint16, 5);
			GCHL_GENERATE(uint32, 10);
			GCHL_GENERATE(uint64, 20);
			GCHL_GENERATE(float, 50);
			GCHL_GENERATE(double, 350);
			GCHL_GENERATE(bool, 5);
#undef GCHL_GENERATE

			// allow to use l-value-reference operator overloads with r-value-reference Stringizer
			template<class T>
			CAGE_FORCE_INLINE constexpr StringizerBase<N> &operator+(const T &other) &&
			{
				return *this + other;
			}
		};

		template<uint32 N>
		template<uint32 M>
		CAGE_FORCE_INLINE constexpr StringBase<N>::StringBase(const StringizerBase<M> &other) : StringBase(other.value)
		{}
	}

	// delegates

	template<class T>
	struct Delegate;

	template<class R, class... Ts>
	struct Delegate<R(Ts...)>
	{
	private:
		R (*fnc)(void *, Ts...) = nullptr;
		void *inst = nullptr;

	public:
		constexpr Delegate() noexcept = default;
		constexpr Delegate(const Delegate &) noexcept = default;
		constexpr Delegate &operator=(const Delegate &) noexcept = default;

		template<class Callable>
		requires(std::is_invocable_r_v<R, Callable, Ts...> && !std::is_same_v<std::remove_cvref_t<Callable>, std::remove_cvref_t<Delegate<R(Ts...)>>>)
		Delegate(Callable &&callable) noexcept
		{
			bind(std::move(callable));
		}

		template<class Callable>
		requires(std::is_invocable_r_v<R, Callable, Ts...> && !std::is_same_v<std::remove_cvref_t<Callable>, std::remove_cvref_t<Delegate<R(Ts...)>>>)
		Delegate &bind(Callable &&callable) noexcept
		{
			auto l = [cl = std::move(callable)](Ts... vs) { return (cl)(std::forward<Ts>(vs)...); };
			using L = decltype(l);
			static_assert(sizeof(L) <= sizeof(void *) && std::is_trivially_copyable_v<L> && std::is_trivially_destructible_v<L>);
			fnc = +[](void *inst, Ts... vs) -> R
			{
				L l = *(L *)(&inst);
				return (l)(std::forward<Ts>(vs)...);
			};
			detail::memcpy(&inst, &l, sizeof(L));
			return *this;
		}

		template<R (*F)(Ts...)>
		constexpr Delegate &bind() noexcept
		{
			fnc = +[](void *inst, Ts... vs) -> R { return (F)(std::forward<Ts>(vs)...); };
			return *this;
		}

		template<class D, R (*F)(D, Ts...)>
		requires(sizeof(D) <= sizeof(void *) && std::is_trivially_copyable_v<D> && std::is_trivially_destructible_v<D>)
		Delegate &bind(D d) noexcept
		{
			static_assert(sizeof(D) > 0);
			fnc = +[](void *inst, Ts... vs) -> R
			{
				D d;
				detail::memcpy(&d, &inst, sizeof(D));
				return (F)(d, std::forward<Ts>(vs)...);
			};
			detail::memcpy(&inst, &d, sizeof(D));
			return *this;
		}

		template<class C, R (C::*F)(Ts...)>
		Delegate &bind(C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) -> R { return (((C *)inst)->*F)(std::forward<Ts>(vs)...); };
			inst = i;
			return *this;
		}

		template<class C, R (C::*F)(Ts...) const>
		Delegate &bind(const C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) -> R { return (((const C *)inst)->*F)(std::forward<Ts>(vs)...); };
			inst = const_cast<C *>(i);
			return *this;
		}

		CAGE_FORCE_INLINE constexpr explicit operator bool() const noexcept { return !!fnc; }

		CAGE_FORCE_INLINE constexpr void clear() noexcept
		{
			inst = nullptr;
			fnc = nullptr;
		}

		CAGE_FORCE_INLINE constexpr R operator()(Ts... vs) const
		{
			CAGE_ASSERT(!!*this);
			return fnc(inst, std::forward<Ts>(vs)...);
		}

		constexpr bool operator==(const Delegate &) const noexcept = default;
	};

	// holder

	template<class T>
	struct Holder;

	namespace privat
	{
		struct CAGE_CORE_API HolderControlBase : private Immovable
		{
		private:
			alignas(sizeof(void *)) volatile uint32 counter = 0;

		public:
			Delegate<void(void *)> deleter;
			void *deletee = nullptr;
			void inc();
			void dec();
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

			HolderBase(HolderBase &&other) noexcept : HolderBase(other.data_, std::move(other)) {}

			HolderBase &operator=(HolderBase &&other) noexcept
			{
				HolderBase tmp(other.share());
				clear();
				std::swap(data_, tmp.data_);
				std::swap(control_, tmp.control_);
				return *this;
			}

			~HolderBase() noexcept
			{
				try
				{
					clear();
				}
				catch (...)
				{
					detail::logCurrentCaughtException();
					detail::irrecoverableError("exception in ~HolderBase");
				}
			}

			CAGE_FORCE_INLINE explicit operator bool() const noexcept { return !!data_; }

			CAGE_FORCE_INLINE T *operator->() const
			{
				CAGE_ASSERT(data_);
				return data_;
			}

			template<bool Enable = !std::is_same_v<T, void>>
			CAGE_FORCE_INLINE typename std::enable_if_t<Enable, T> &operator*() const
			{
				CAGE_ASSERT(data_);
				return *data_;
			}

			CAGE_FORCE_INLINE T *get() const noexcept { return data_; }

			CAGE_FORCE_INLINE T *operator+() const noexcept { return data_; }

			void clear()
			{
				data_ = nullptr;
				HolderControlBase *tmpCtrl = control_;
				control_ = nullptr;
				// decrementing the counter is purposefully deferred to until this holder is cleared first
				if (tmpCtrl)
					tmpCtrl->dec();
			}

			CAGE_FORCE_INLINE Holder<T> share() const { return Holder<T>(data_, control_); }

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
			M *m = dynamic_cast<M *>(this->data_);
			if (!m && this->data_)
				CAGE_THROW_ERROR(Exception, "bad dynamic cast");
			return Holder<M>(m, std::move(*this));
		}

		template<class M>
		Holder<M> cast() &&
		{
			return Holder<M>(static_cast<M *>(this->data_), std::move(*this));
		}

		CAGE_FORCE_INLINE operator Holder<const T>() && { return Holder<const T>((const T *)this->data_, std::move(*this)); }
	};

	template<class T>
	struct Holder<PointerRange<T>> : public privat::HolderBase<PointerRange<T>>
	{
		using privat::HolderBase<PointerRange<T>>::HolderBase;
		using size_type = typename PointerRange<T>::size_type;
		using value_type = typename PointerRange<T>::value_type;

		CAGE_FORCE_INLINE T *begin() const noexcept { return this->data_ ? this->data_->begin() : nullptr; }
		CAGE_FORCE_INLINE T *end() const noexcept { return this->data_ ? this->data_->end() : nullptr; }
		CAGE_FORCE_INLINE T *data() const noexcept { return begin(); }
		CAGE_FORCE_INLINE size_type size() const noexcept { return end() - begin(); }
		CAGE_FORCE_INLINE bool empty() const noexcept { return size() == 0; }
		CAGE_FORCE_INLINE T &operator[](size_type idx) const
		{
			CAGE_ASSERT(idx < size());
			return begin()[idx];
		}

		template<class M>
		requires(std::is_same_v<std::remove_cv_t<M>, void>)
		Holder<void> cast() &&
		{
			return Holder<void>((void *)this->data_, std::move(*this));
		}

		template<class M>
		requires(!std::is_same_v<std::remove_cv_t<M>, void>)
		Holder<PointerRange<M>> cast() &&
		{
			this->data_->template cast<M>(); // verify preconditions
			return Holder<PointerRange<M>>((PointerRange<M> *)this->data_, std::move(*this));
		}

		CAGE_FORCE_INLINE operator Holder<PointerRange<const T>>() && { return Holder<PointerRange<const T>>((PointerRange<const T> *)this->data_, std::move(*this)); }
	};

	// memory arena

	namespace privat
	{
		struct OperatorNewTrait
		{};
		struct MemoryArenaStub;
	}
}

CAGE_FORCE_INLINE void *operator new(std::size_t size, void *ptr, cage::privat::OperatorNewTrait) noexcept
{
	return ptr;
}
CAGE_FORCE_INLINE void *operator new[](std::size_t size, void *ptr, cage::privat::OperatorNewTrait) noexcept
{
	return ptr;
}
CAGE_FORCE_INLINE void operator delete(void *ptr, void *ptr2, cage::privat::OperatorNewTrait) noexcept {}
CAGE_FORCE_INLINE void operator delete[](void *ptr, void *ptr2, cage::privat::OperatorNewTrait) noexcept {}

namespace cage
{
	namespace privat
	{
		template<class T, class... Ts>
		requires(std::is_constructible_v<T, Ts...>)
		struct HolderControlMixin : public privat::HolderControlBase
		{
			CAGE_FORCE_INLINE explicit HolderControlMixin(Ts... vs) : data(std::forward<Ts>(vs)...) {}
			T data;
		};
	}

	struct CAGE_CORE_API MemoryArena
	{
	private:
		privat::MemoryArenaStub *stub = nullptr;
		void *inst = nullptr;

	public:
		template<class A>
		inline explicit MemoryArena(A *a) noexcept;

		[[nodiscard]] void *allocate(uintPtr size, uintPtr alignment);
		void deallocate(void *ptr);
		void flush();

		Holder<PointerRange<char>> createBuffer(uintPtr size, uintPtr alignment = 16);

		template<class T, class... Ts>
		requires(std::is_constructible_v<T, Ts...>)
		[[nodiscard]] CAGE_FORCE_INLINE T *createObject(Ts... vs)
		{
			void *ptr = allocate(sizeof(T), alignof(T));
			try
			{
				return new (ptr, privat::OperatorNewTrait()) T(std::forward<Ts>(vs)...);
			}
			catch (...)
			{
				deallocate(ptr);
				throw;
			}
		}

		template<class T, class... Ts>
		requires(std::is_constructible_v<T, Ts...>)
		CAGE_FORCE_INLINE Holder<T> createHolder(Ts... vs)
		{
			using Ctrl = privat::HolderControlMixin<T, Ts...>;
			Ctrl *p = createObject<Ctrl>(std::forward<Ts>(vs)...);
			p->deletee = p;
			p->deleter.template bind<MemoryArena, &MemoryArena::destroy<Ctrl>>(this);
			return Holder<T>(&p->data, p);
		};

		template<class Interface, class Impl, class... Ts>
		requires(std::is_constructible_v<Impl, Ts...>)
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
				detail::irrecoverableError("exception in MemoryArena::destroy");
			}
			deallocate(ptr);
		}

		CAGE_FORCE_INLINE bool operator==(const MemoryArena &other) const noexcept { return inst == other.inst; }
	};

	CAGE_CORE_API MemoryArena &systemMemory();
	CAGE_CORE_API uint64 applicationTime();
}

#endif // guard_core_h_39243ce0_71a5_4900_8898_63fb89591b7b_
