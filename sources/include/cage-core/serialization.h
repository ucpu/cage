#ifndef guard_serialization_h_edsg45df4h654fdr56h4gfd564h
#define guard_serialization_h_edsg45df4h654fdr56h4gfd564h

#include <cage-core/memoryInplaceBuffer.h>

namespace cage
{
	struct MemoryBuffer;

	namespace privat
	{
		struct CAGE_CORE_API SerializationInterface
		{
		public:
			SerializationInterface(PointerRange<char> buffer);
			SerializationInterface(MemoryBuffer &buffer);
			template<uintPtr Capacity>
			SerializationInterface(InplaceBuffer<Capacity> &inplace)
			{
				origin = &inplace;
				getData = +[](void *origin) -> char * { return ((InplaceBuffer<Capacity> *)origin)->data; };
				getSize = +[](void *origin) -> uintPtr { return ((InplaceBuffer<Capacity> *)origin)->size; };
				setSize = +[](void *origin, uintPtr size) -> void { ((InplaceBuffer<Capacity> *)origin)->size = size; };
			}

			using GetData = char *(*)(void *);
			using GetSize = uintPtr (*)(void *);
			using SetSize = void (*)(void *, uintPtr);
			GetData getData = nullptr;
			GetSize getSize = nullptr;
			SetSize setSize = nullptr;
			void *origin = nullptr;
		};

	}

	struct CAGE_CORE_API Serializer : private Noncopyable
	{
		explicit Serializer(PointerRange<char> buffer);
		explicit Serializer(MemoryBuffer &buffer, uintPtr capacity = m);
		template<uintPtr Capacity>
		explicit Serializer(InplaceBuffer<Capacity> &inplace) : Serializer(inplace, 0, Capacity)
		{}
		explicit Serializer(privat::SerializationInterface interface, uintPtr offset, uintPtr capacity);

		uintPtr available() const; // number of bytes still available in the buffer (valid only if the capacity was provided in the constructor)
		void write(PointerRange<const char> buffer);
		PointerRange<char> writeAhead(uintPtr size); // use with care! - future writes may invalidate the pointer
		void writeLine(const String &line);
		Serializer reserve(uintPtr s);

	private:
		privat::SerializationInterface interface;
		uintPtr offset = 0; // current position in the buffer
		uintPtr capacity = 0; // max size of the buffer
	};

	struct CAGE_CORE_API Deserializer : private Noncopyable
	{
		explicit Deserializer(PointerRange<const char> buffer);

		uintPtr available() const; // number of bytes still available in the buffer
		PointerRange<const char> read(uintPtr size);
		void readInto(PointerRange<char> buffer);
		bool readLine(PointerRange<const char> &line);
		bool readLine(String &line);
		Deserializer subview(uintPtr s);
		Deserializer copy() const;

	private:
		explicit Deserializer(const char *data, uintPtr offset, uintPtr size);

		const char *data = nullptr;
		uintPtr offset = 0; // current position in the buffer
		uintPtr size = 0; // max size of the buffer
	};

	// helpers

	// reinterpret types of range of elements
	// preserves number of bytes, but may change number of elements
	template<class Dst = const char, class Src>
	requires(std::is_trivially_copyable_v<Dst> && std::is_trivially_copyable_v<Src>)
	constexpr PointerRange<Dst> bufferCast(const PointerRange<Src> src)
	{
		return { reinterpret_cast<Dst *>(src.begin()), reinterpret_cast<Dst *>(src.end()) };
	}

	// make range of a single object
	template<class T>
	constexpr PointerRange<T> rangeView(T &object)
	{
		return { &object, &object + 1 };
	}

	// reinterpret single object as range of bytes
	template<class Dst = const char, class Src>
	constexpr PointerRange<Dst> bufferView(Src &object)
	{
		return bufferCast<Dst, Src>(rangeView<Src>(object));
	}

	// general serialization

	namespace privat
	{
		template<class>
		struct IsPointerRange : std::false_type
		{};
		template<class V>
		struct IsPointerRange<PointerRange<V>> : std::true_type
		{};

		template<class>
		struct IsStdVariant : std::false_type
		{};
		template<class... Ts>
		struct IsStdVariant<std::variant<Ts...>> : std::true_type
		{};
	}

	namespace serialization
	{
		template<class T>
		struct Writable : std::bool_constant<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T> && !privat::IsPointerRange<T>::value && !privat::IsStdVariant<T>::value>
		{};
	}

	namespace privat
	{
		template<class T>
		concept WritableConcept = serialization::Writable<T>::value;
	}

	Serializer &operator<<(Serializer &s, const privat::WritableConcept auto &v)
	{
		s.write(bufferView<const char>(v));
		return s;
	}

	Deserializer &operator>>(Deserializer &s, privat::WritableConcept auto &v)
	{
		s.readInto(bufferView<char>(v));
		return s;
	}

	// r-value deserializer

	template<class T>
	Deserializer &&operator>>(Deserializer &&s, T &v)
	{
		s >> v;
		return std::move(s);
	}

	// overloads for c array

	template<class T, uintPtr N>
	requires(privat::WritableConcept<T>)
	Serializer &operator<<(Serializer &s, const T (&v)[N])
	{
		for (auto &it : v)
			s << it;
		return s;
	}

	template<class T, uintPtr N>
	requires(privat::WritableConcept<T>)
	Deserializer &operator>>(Deserializer &s, T (&v)[N])
	{
		for (auto &it : v)
			s >> it;
		return s;
	}

	// overloads for StringBase

	template<uint32 N>
	Serializer &operator<<(Serializer &s, const detail::StringBase<N> &v)
	{
		s << v.length();
		s.write(v);
		return s;
	}

	template<uint32 N>
	Deserializer &operator>>(Deserializer &s, detail::StringBase<N> &v)
	{
		decltype(v.length()) size = 0;
		s >> size;
		v = detail::StringBase<N>(s.read(size));
		return s;
	}

	// overloads for std::pair

	template<class A, class B>
	Serializer &operator<<(Serializer &s, const std::pair<A, B> &p)
	{
		s << p.first << p.second;
		return s;
	}

	template<class A, class B>
	Deserializer &operator>>(Deserializer &s, std::pair<A, B> &p)
	{
		s >> const_cast<std::remove_const_t<A> &>(p.first) >> p.second;
		return s;
	}

	// overloads for MemoryBuffer

	CAGE_CORE_API Serializer &operator<<(Serializer &s, const MemoryBuffer &b);
	CAGE_CORE_API Deserializer &operator>>(Deserializer &d, MemoryBuffer &b);
}

#endif
