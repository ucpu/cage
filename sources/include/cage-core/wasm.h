#ifndef guard_wasm_h_vhj1b548hfdt74h
#define guard_wasm_h_vhj1b548hfdt74h

#include <cage-core/core.h>

namespace cage
{
	struct WasmInfo
	{
		String moduleName;
		String name;
		String kind;
	};

	class CAGE_CORE_API WasmModule : private Immovable
	{
	public:
		Holder<PointerRange<WasmInfo>> importInfo() const;
		Holder<PointerRange<WasmInfo>> exportInfo() const;
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
		enum class WasmValEnum
		{
			Unknown,
			Void,
			Int32,
			Int64,
			Float,
			Double,
		};

		template<class T>
		CAGE_FORCE_INLINE bool matchingWasmType(WasmValEnum v)
		{
			if constexpr (std::is_same_v<T, void>)
				return v == WasmValEnum::Void;
			if constexpr (std::is_same_v<T, sint32> || std::is_same_v<T, uint32>)
				return v == WasmValEnum::Int32;
			if constexpr (std::is_same_v<T, sint64> || std::is_same_v<T, uint64>)
				return v == WasmValEnum::Int64;
			if constexpr (std::is_same_v<T, float>)
				return v == WasmValEnum::Float;
			if constexpr (std::is_same_v<T, double>)
				return v == WasmValEnum::Double;
			return false;
		}

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

		template<class T, class... U>
		concept AnyOfConcept = (std::same_as<T, U> || ...);
		template<class T>
		concept WasmCompatibleConcept = AnyOfConcept<T, sint32, uint32, sint64, uint64, float, double, void>;
	}

	template<class T>
	struct WasmFunction;

	template<privat::WasmCompatibleConcept R, privat::WasmCompatibleConcept... Ts>
	struct WasmFunction<R(Ts...)> : private Noncopyable
	{
	public:
		WasmFunction(Holder<privat::WasmFunctionInternal> &&func_) : func(std::move(func_))
		{
			CAGE_ASSERT(func);
			try
			{
				if (!matchingWasmType<R>(func->returns()))
					CAGE_THROW_ERROR(Exception, "mismatched return type in wasm function");
				const auto ps = func->params();
				if (ps.size() != sizeof...(Ts))
					CAGE_THROW_ERROR(Exception, "mismatched number of parameters in wasm function");
				const auto &check = [&]<class T>(uint32 i, T) -> void
				{
					if (!matchingWasmType<T>(ps[i]))
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
