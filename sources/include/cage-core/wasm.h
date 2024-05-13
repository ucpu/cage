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
	};

	CAGE_CORE_API Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> module);

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
			if constexpr (std::is_same_v<T, sint32>)
				return v == WasmValEnum::Int32;
			if constexpr (std::is_same_v<T, sint64>)
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
			PointerRange<const uint32> offsets() const; // number of _sint32 elements_ in data to skip for particular parameter
			PointerRange<uint32> data();
			void call();
		};

		CAGE_CORE_API Holder<WasmFunctionInternal> wasmFindFunction(Holder<WasmInstance> &&instance, const String &name);

		template<class T, class... U>
		concept AnyOfConcept = (std::same_as<T, U> || ...);
		template<class T>
		concept WasmCompatibleConcept = AnyOfConcept<T, sint32, sint64, float, double, void>;
	}

	template<class T>
	class WasmFunction;

	template<privat::WasmCompatibleConcept R, privat::WasmCompatibleConcept... Ts>
	class WasmFunction<R(Ts...)> : private Noncopyable
	{
	public:
		WasmFunction(Holder<privat::WasmFunctionInternal> &&func_) : func(std::move(func_))
		{
			CAGE_ASSERT(func);
			try
			{
				if (!matchingWasmType<R>(func->returns()))
					CAGE_THROW_ERROR(Exception, "mismatched size of return value in wasm function");
				const auto ps = func->params();
				if (ps.size() != sizeof...(Ts))
					CAGE_THROW_ERROR(Exception, "mismatched number of parameters in wasm function");
				const auto &check = [&]<class T>(uint32 i, T) -> void
				{
					if (!matchingWasmType<T>(ps[i]))
						CAGE_THROW_ERROR(Exception, "mismatched size of parameter in wasm function");
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
			PointerRange<const uint32> offsets = func->offsets();

			const auto &fill = [&]<class T>(uint32 i, T v) -> void { *(T *)(void *)(data.data() + offsets[i]) = v; };
			uint32 i = 0;
			(fill(i++, std::forward<Ts>(vs)), ...);

			func->call();

			if constexpr (!std::is_same_v<R, void>)
				return *(R *)(void *)data.data();
		}

	private:
		Holder<privat::WasmFunctionInternal> func;
	};
}

#endif // guard_wasm_h_vhj1b548hfdt74h
