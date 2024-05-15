#ifndef guard_wasm_h_vhj1b548hfdt74h
#define guard_wasm_h_vhj1b548hfdt74h

#include <cage-core/core.h>

namespace cage
{
	class WasmInstance;

	namespace privat
	{
		template<class T, class... U>
		concept WasmAnyOfConcept = (std::same_as<T, U> || ...);
		template<class T>
		concept WasmParameterConcept = WasmAnyOfConcept<T, sint32, uint32, sint64, uint64, float, double, void>;

		enum class WasmValEnum : uint8
		{
			Unknown,
			Void,
			Int32,
			Int64,
			Float,
			Double,
		};

		template<WasmParameterConcept T>
		consteval WasmValEnum wasmType()
		{
			if constexpr (std::is_same_v<T, void>)
				return WasmValEnum::Void;
			if constexpr (std::is_same_v<T, sint32> || std::is_same_v<T, uint32>)
				return WasmValEnum::Int32;
			if constexpr (std::is_same_v<T, sint64> || std::is_same_v<T, uint64>)
				return WasmValEnum::Int64;
			if constexpr (std::is_same_v<T, float>)
				return WasmValEnum::Float;
			if constexpr (std::is_same_v<T, double>)
				return WasmValEnum::Double;
			return WasmValEnum::Unknown;
		}

		struct CAGE_CORE_API WasmTypelessDelegate
		{
			void *fnc = nullptr;
			void *inst = nullptr;
		};

		struct CAGE_CORE_API WasmNative
		{
			detail::StringBase<59> name;
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
						case WasmValEnum::Int32:
							return "i";
						case WasmValEnum::Int64:
							return "I";
						case WasmValEnum::Float:
							return "f";
						case WasmValEnum::Double:
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
		void add(Delegate<R(WasmInstance *, Ts...)> function, const detail::StringBase<59> &name)
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
		void add(Delegate<R(Ts...)> function, const detail::StringBase<59> &name)
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

		void commit(const detail::StringBase<59> &moduleName = "env");

	private:
		void add_(privat::WasmNative fnc);
	};

	CAGE_CORE_API Holder<WasmNatives> newWasmNatives();

	struct WasmInfo
	{
		detail::StringBase<59> moduleName;
		detail::StringBase<59> name;
		detail::StringBase<27> kind;
		bool linked = false;
	};

	class CAGE_CORE_API WasmModule : private Immovable
	{
	public:
		Holder<PointerRange<WasmInfo>> importsInfo() const;
		Holder<PointerRange<WasmInfo>> exportsInfo() const;
	};

	CAGE_CORE_API Holder<WasmModule> newWasmModule(PointerRange<const char> buffer, bool allowAot = false);

	class CAGE_CORE_API WasmInstance : private Immovable
	{
	public:
		Holder<WasmModule> module() const;
		void *wasm2native(uint32 address);
		uint32 native2wasm(void *address);
	};

	CAGE_CORE_API Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> &&module);

	class CAGE_CORE_API WasmBuffer : private Immovable
	{
	public:
		Holder<WasmInstance> instance() const;
		uint32 size() const;
		uint32 wasmAddr() const;
		void *nativeAddr();
		PointerRange<char> buffer();
	};

	CAGE_CORE_API Holder<WasmBuffer> wasmAllocate(Holder<WasmInstance> &&instance, uint32 size);

	namespace privat
	{
		class CAGE_CORE_API WasmFunctionInternal : private Immovable
		{
		public:
			Holder<WasmInstance> instance() const;
			String name() const;
			WasmValEnum returns() const;
			PointerRange<const WasmValEnum> params() const;
			PointerRange<uint32> data();
			void call();
		};

		CAGE_CORE_API Holder<WasmFunctionInternal> wasmFindFunction(Holder<WasmInstance> &&instance, const String &name);
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
}

#endif // guard_wasm_h_vhj1b548hfdt74h
