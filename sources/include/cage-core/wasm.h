#ifndef guard_wasm_h_vhj1b548hfdt74h
#define guard_wasm_h_vhj1b548hfdt74h

#include <cage-core/core.h>

namespace cage
{
	class WasmInstance;

	using WasmName = detail::StringBase<59>;

	namespace privat
	{
		template<class T, class... U>
		concept WasmAnyOfConcept = (std::same_as<T, U> || ...);
		template<class T>
		concept WasmParameterConcept = WasmAnyOfConcept<T, sint32, uint32, sint64, uint64, float, double, void>;

		enum class WasmTypeEnum : uint8
		{
			Unknown,
			Void,
			Int32,
			Int64,
			Float,
			Double,
		};

		template<WasmParameterConcept T>
		consteval WasmTypeEnum wasmType()
		{
			if constexpr (std::is_same_v<T, void>)
				return WasmTypeEnum::Void;
			if constexpr (std::is_same_v<T, sint32> || std::is_same_v<T, uint32>)
				return WasmTypeEnum::Int32;
			if constexpr (std::is_same_v<T, sint64> || std::is_same_v<T, uint64>)
				return WasmTypeEnum::Int64;
			if constexpr (std::is_same_v<T, float>)
				return WasmTypeEnum::Float;
			if constexpr (std::is_same_v<T, double>)
				return WasmTypeEnum::Double;
			return WasmTypeEnum::Unknown;
		}

		struct CAGE_CORE_API WasmTypelessDelegate
		{
			void *fnc = nullptr;
			void *inst = nullptr;
		};

		struct CAGE_CORE_API WasmNative
		{
			WasmName name;
			detail::StringBase<27> signature;
			WasmTypelessDelegate delegate;
			void *funcPtr = nullptr;

			template<WasmParameterConcept R, WasmParameterConcept... Ts>
			void prepare()
			{
				static_assert(sizeof...(Ts) + 3 <= WasmNative().signature.MaxLength);
				const auto &toChar = []<class T>(T) -> const char *
				{
					switch (wasmType<T>())
					{
						case WasmTypeEnum::Int32:
							return "i";
						case WasmTypeEnum::Int64:
							return "I";
						case WasmTypeEnum::Float:
							return "f";
						case WasmTypeEnum::Double:
							return "F";
						default:
							return "";
					};
				};
				signature += "(";
				((signature += toChar(Ts{})), ...);
				signature += ")";
				if constexpr (!std::is_same_v<R, void>)
					signature += toChar(R{});
			}
		};

		CAGE_CORE_API const WasmTypelessDelegate *wasmCallbackAttachment(void *ctx);
		CAGE_CORE_API WasmInstance *wasmCallbackInstance(void *ctx);
	}

	class CAGE_CORE_API WasmNatives : private Immovable
	{
	public:
		template<privat::WasmParameterConcept R, privat::WasmParameterConcept... Ts>
		void add(Delegate<R(WasmInstance *, Ts...)> function, const WasmName &name)
		{
			using namespace privat;
			WasmNative f;
			f.name = name;
			f.prepare<R, Ts...>();
			static_assert(sizeof(function) == sizeof(WasmTypelessDelegate));
			f.delegate = *(WasmTypelessDelegate *)&function;
			f.funcPtr = (void *)+[](void *ctx, Ts... vs) -> R
			{
				Delegate<R(WasmInstance *, Ts...)> function = *(const Delegate<R(WasmInstance *, Ts...)> *)wasmCallbackAttachment(ctx);
				WasmInstance *instance = wasmCallbackInstance(ctx);
				return function(instance, std::forward<Ts>(vs)...);
			};
			add_(f);
		}

		template<privat::WasmParameterConcept R, privat::WasmParameterConcept... Ts>
		void add(Delegate<R(Ts...)> function, const WasmName &name)
		{
			using namespace privat;
			WasmNative f;
			f.name = name;
			f.prepare<R, Ts...>();
			static_assert(sizeof(function) == sizeof(WasmTypelessDelegate));
			f.delegate = *(WasmTypelessDelegate *)&function;
			f.funcPtr = (void *)+[](void *ctx, Ts... vs) -> R
			{
				Delegate<R(Ts...)> function = *(const Delegate<R(Ts...)> *)wasmCallbackAttachment(ctx);
				return function(std::forward<Ts>(vs)...);
			};
			add_(f);
		}

		void commit(const WasmName &moduleName = "env");

	private:
		void add_(privat::WasmNative fnc);
	};

	CAGE_CORE_API Holder<WasmNatives> newWasmNatives();

	struct CAGE_CORE_API WasmSymbol
	{
		WasmName moduleName;
		WasmName name;
		detail::StringBase<27> kind;
		bool linked = false;
	};

	class CAGE_CORE_API WasmModule : private Immovable
	{
	public:
		Holder<PointerRange<WasmSymbol>> symbolsImports() const;
		Holder<PointerRange<WasmSymbol>> symbolsExports() const;
	};

	CAGE_CORE_API Holder<WasmModule> newWasmModule(PointerRange<const char> buffer, bool allowAot = false);

	class CAGE_CORE_API WasmBuffer : private Noncopyable
	{
		friend class WasmInstance;

	public:
		WasmBuffer() = default;
		WasmBuffer(WasmInstance *);
		WasmBuffer(WasmBuffer &&);
		WasmBuffer &operator=(WasmBuffer &&);
		~WasmBuffer();

		WasmInstance *instance() const { return instance_; }
		bool owned() const { return owned_; }
		void disown() { owned_ = false; }

		uint32 wasmAddr() const { return wasmPtr_; }
		void *nativeAddr() const;

		uint32 size() const { return size_; }
		uint32 capacity() const { return capacity_; }
		void resize(uint32 s);
		void free();

		PointerRange<char> buffer()
		{
			char *p = (char *)nativeAddr();
			return { p, p + size_ };
		}
		PointerRange<const char> buffer() const
		{
			const char *p = (const char *)nativeAddr();
			return { p, p + size_ };
		}

		void write(PointerRange<const char> buffer);

		template<uint32 N = String::MaxLength>
		void write(const detail::StringBase<N> &str)
		{
			const char *p = str.c_str();
			write(p, str.length() + 1);
		}

		template<uint32 N = String::MaxLength>
		detail::StringBase<N> read() const
		{
			detail::StringBase<N> s(buffer());
			while (s.length() && s[s.length() - 1] == '\0')
				s.rawLength()--;
			return s;
		}

	private:
		WasmInstance *instance_ = nullptr;
		uint32 size_ = 0;
		uint32 capacity_ = 0;
		uint32 wasmPtr_ = 0;
		bool owned_ = false;
	};

	namespace privat
	{
		class CAGE_CORE_API WasmFunctionInternal : private Immovable
		{
		public:
			WasmInstance *instance() const;
			WasmName name() const;
			WasmTypeEnum returns() const;
			PointerRange<const WasmTypeEnum> params() const;
			PointerRange<uint32> data();
			void call();
		};
	}

	template<class T>
	struct WasmFunction;

	template<privat::WasmParameterConcept R, privat::WasmParameterConcept... Ts>
	struct WasmFunction<R(Ts...)> : private Noncopyable
	{
	public:
		WasmFunction(Holder<privat::WasmFunctionInternal> &&func_) : func(std::move(func_))
		{
			using namespace privat;
			CAGE_ASSERT(func);
			try
			{
				if (wasmType<R>() != func->returns())
					CAGE_THROW_ERROR(Exception, "mismatched return type in wasm function");
				const auto ps = func->params();
				if (ps.size() != sizeof...(Ts))
					CAGE_THROW_ERROR(Exception, "mismatched number of parameters in wasm function");
				const auto &check = [&]<class T>(uint32 i, T) -> void
				{
					if (wasmType<T>() != ps[i])
						CAGE_THROW_ERROR(Exception, "mismatched parameter type in wasm function");
				};
				uint32 i = 0;
				(check(i++, Ts{}), ...);
			}
			catch (...)
			{
				CAGE_LOG_THROW(func->name());
				throw;
			}
		}

		R operator()(Ts... vs)
		{
			PointerRange<uint32> data = func->data();

			uint32 offset = 0;
			const auto &fill = [&]<class T>(T v) -> void
			{
				*(T *)(void *)(data.data() + offset) = v;
				offset += sizeof(T) / 4;
			};
			(fill(std::forward<Ts>(vs)), ...);

			func->call();

			if constexpr (!std::is_same_v<R, void>)
				return *(R *)(void *)data.data();
		}

	private:
		Holder<privat::WasmFunctionInternal> func;
	};

	class CAGE_CORE_API WasmInstance : private Immovable
	{
	public:
		Holder<WasmModule> module() const;

		void *wasm2native(uint32 address, uint32 size = 0);
		uint32 native2wasm(void *address, uint32 size = 0);
		uint32 malloc(uint32 size);
		void free(uint32 wasmAddr);
		uint32 strLen(uint32 wasmAddr);
		WasmBuffer allocate(uint32 size, uint32 capacity = 0, bool owned = true);
		WasmBuffer adapt(uint32 wasmAddr, uint32 size, bool owned = false);

		template<class T>
		CAGE_FORCE_INLINE WasmFunction<T> function(const WasmName &name)
		{
			return WasmFunction<T>(function_(name));
		}

	private:
		Holder<privat::WasmFunctionInternal> function_(const WasmName &name);
	};

	CAGE_CORE_API Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> &&module);
}

#endif // guard_wasm_h_vhj1b548hfdt74h
