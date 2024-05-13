#include <cmath> // max
#include <cstdarg> // va_list
#include <cstdio> // vsnprintf
#include <cstring> // strlen
#include <vector>

#include <wasm_export.h>

#include <cage-core/pointerRangeHolder.h>
#include <cage-core/wasm.h>

extern "C"
{
	using namespace cage;

	int cage_wamr_vprintf(const char *format, std::va_list ap)
	{
		String str;
		int res = std::vsnprintf(str.rawData(), str.MaxLength - 100, format, ap);
		str.rawLength() = std::strlen(str.rawData());
		CAGE_LOG(SeverityEnum::Info, "wasm-print", str);
		return res;
	}

	void cage_wamr_log(uint32 log_level, const char *file, int line, const char *fmt, ...)
	{
		// silence wamr catching our c++ exceptions
		//CAGE_LOG(SeverityEnum::Info, "wasm-log", fmt);
		// todo
	}
}

namespace cage
{
	namespace
	{
		class WasmModuleImpl : public WasmModule
		{
		public:
			Holder<PointerRange<char>> buffer;
			wasm_module_t module = nullptr;

			WasmModuleImpl(PointerRange<const char> buffer_, bool allowAot)
			{
				switch (get_package_type((uint8_t *)buffer_.data(), buffer_.size()))
				{
					case Wasm_Module_Bytecode:
						break; // always ok
					case Wasm_Module_AoT:
						if (allowAot)
							break; // ok if allowed
						[[fallthrough]];
					case Package_Type_Unknown:
					default:
						CAGE_THROW_ERROR(Exception, "unknown or disallowed wasm package type");
						break;
				}

				// must keep copy of the buffer
				buffer = systemMemory().createBuffer(buffer_.size());
				detail::memcpy(buffer.data(), buffer_.data(), buffer.size());

				String err;
				module = wasm_runtime_load((uint8_t *)buffer.data(), buffer.size(), err.rawData(), err.MaxLength - 100);
				if (!module)
				{
					err.rawLength() = std::strlen(err.rawData());
					CAGE_LOG_CONTINUE(SeverityEnum::Note, "wasm", err);
					CAGE_THROW_ERROR(Exception, "failed loading wasm module");
				}
			}

			~WasmModuleImpl()
			{
				if (module)
				{
					wasm_runtime_unload(module);
					module = nullptr;
				}
			}
		};

		class WasmInstanceImpl : public WasmInstance
		{
		public:
			const Holder<WasmModuleImpl> module;
			wasm_module_inst_t instance = nullptr;
			wasm_exec_env_t exec = nullptr;

			WasmInstanceImpl(Holder<WasmModule> &&module_) : module(std::move(module_).cast<WasmModuleImpl>())
			{
				String err;
				instance = wasm_runtime_instantiate(module->module, 1'000'000, 1'000'000, err.rawData(), err.MaxLength - 100);
				if (!instance)
				{
					err.rawLength() = std::strlen(err.rawData());
					CAGE_LOG_CONTINUE(SeverityEnum::Note, "wasm", err);
					CAGE_THROW_ERROR(Exception, "failed instantiating wasm module");
				}
				exec = wasm_runtime_get_exec_env_singleton(instance);
				CAGE_ASSERT(exec);
			}

			~WasmInstanceImpl()
			{
				if (instance)
				{
					wasm_runtime_deinstantiate(instance);
					instance = nullptr;
				}
			}
		};

		void initialize()
		{
			static int dummy = []() -> int
			{
				if (!wasm_runtime_init())
					CAGE_THROW_ERROR(Exception, "failed to initialize wasm runtime");
				return 0;
			}();
		}

		String toString(wasm_import_export_kind_t k)
		{
			switch (k)
			{
				case WASM_IMPORT_EXPORT_KIND_FUNC:
					return "function";
				case WASM_IMPORT_EXPORT_KIND_TABLE:
					return "table";
				case WASM_IMPORT_EXPORT_KIND_MEMORY:
					return "memory";
				case WASM_IMPORT_EXPORT_KIND_GLOBAL:
					return "global";
				default:
					return "unknown";
			}
		}
	}

	Holder<PointerRange<WasmInfo>> WasmModule::importInfo() const
	{
		const WasmModuleImpl *impl = (const WasmModuleImpl *)this;
		const uint32 cnt = wasm_runtime_get_import_count(impl->module);
		PointerRangeHolder<WasmInfo> arr;
		arr.reserve(cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			wasm_import_t t = {};
			wasm_runtime_get_import_type(impl->module, i, &t);
			WasmInfo w;
			w.moduleName = t.module_name;
			w.name = t.name;
			w.kind = toString(t.kind);
			arr.push_back(w);
		}
		return arr;
	}

	Holder<PointerRange<WasmInfo>> WasmModule::exportInfo() const
	{
		const WasmModuleImpl *impl = (const WasmModuleImpl *)this;
		const uint32 cnt = wasm_runtime_get_export_count(impl->module);
		PointerRangeHolder<WasmInfo> arr;
		arr.reserve(cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			wasm_export_t t = {};
			wasm_runtime_get_export_type(impl->module, i, &t);
			WasmInfo w;
			w.name = t.name;
			w.kind = toString(t.kind);
			arr.push_back(w);
		}
		return arr;
	}

	Holder<WasmModule> newWasmModule(PointerRange<const char> buffer, bool allowAot)
	{
		initialize();
		return systemMemory().createImpl<WasmModule, WasmModuleImpl>(buffer, allowAot);
	}

	Holder<WasmModule> WasmInstance::module() const
	{
		const WasmInstanceImpl *impl = (const WasmInstanceImpl *)this;
		return impl->module.share().cast<WasmModule>();
	}

	Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> module)
	{
		CAGE_ASSERT(module);
		return systemMemory().createImpl<WasmInstance, WasmInstanceImpl>(std::move(module));
	}

	namespace privat
	{
		namespace
		{
			WasmValEnum convert(wasm_valkind_t k)
			{
				switch (k)
				{
					case WASM_I32:
						return WasmValEnum::Int32;
					case WASM_I64:
						return WasmValEnum::Int64;
					case WASM_F32:
						return WasmValEnum::Float;
					case WASM_F64:
						return WasmValEnum::Double;
					default:
						return WasmValEnum::Unknown;
				}
			}

			uint32 countOfWasmVal(WasmValEnum v)
			{
				switch (v)
				{
					case WasmValEnum::Void:
					case WasmValEnum::Unknown:
						return 0;
					case WasmValEnum::Int32:
					case WasmValEnum::Float:
						return 1;
					case WasmValEnum::Int64:
					case WasmValEnum::Double:
						return 2;
				}
				return 0;
			}

			class WasmFunctionImpl : public privat::WasmFunctionInternal
			{
			public:
				const String name;
				const Holder<WasmInstanceImpl> instance;
				wasm_function_inst_t func = nullptr;

				WasmValEnum returns = WasmValEnum::Void;
				std::vector<WasmValEnum> params;
				std::vector<uint32> offsets;
				std::vector<uint32> data;

				WasmFunctionImpl(Holder<WasmInstance> &&instance_, const String &name_) : name(name_), instance(std::move(instance_).cast<WasmInstanceImpl>())
				{
					func = wasm_runtime_lookup_function(instance->instance, name.c_str());
					if (!func)
						CAGE_THROW_ERROR(Exception, "wasm function not found");

					const uint32 rc = wasm_func_get_result_count(func, instance->instance);
					if (rc >= 2)
						CAGE_THROW_ERROR(Exception, "wasm function returns multiple values");
					if (rc)
					{
						wasm_valkind_t t = {};
						wasm_func_get_result_types(func, instance->instance, &t);
						returns = convert(t);
					}

					const uint32 pc = wasm_func_get_param_count(func, instance->instance);
					params.reserve(pc);
					offsets.reserve(pc);
					std::vector<wasm_valkind_t> vt;
					vt.resize(pc);
					wasm_func_get_param_types(func, instance->instance, vt.data());
					uint32 offset = 0;
					for (wasm_valkind_t t : vt)
					{
						const WasmValEnum v = convert(t);
						params.push_back(v);
						offsets.push_back(offset);
						offset += countOfWasmVal(v);
					}

					data.resize(std::max(offset, countOfWasmVal(returns)));
				}

				~WasmFunctionImpl() {}
			};
		}

		Holder<WasmInstance> WasmFunctionInternal::instance() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->instance.share().cast<WasmInstance>();
		}

		String WasmFunctionInternal::name() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->name;
		}

		WasmValEnum WasmFunctionInternal::returns() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->returns;
		}

		PointerRange<const WasmValEnum> WasmFunctionInternal::params() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->params;
		}

		PointerRange<const uint32> WasmFunctionInternal::offsets() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->offsets;
		}

		PointerRange<uint32> WasmFunctionInternal::data()
		{
			WasmFunctionImpl *impl = (WasmFunctionImpl *)this;
			return impl->data;
		}

		void WasmFunctionInternal::call()
		{
			WasmFunctionImpl *impl = (WasmFunctionImpl *)this;
			if (!wasm_runtime_call_wasm(impl->instance->exec, impl->func, impl->data.size(), impl->data.data()))
			{
				const String e = wasm_runtime_get_exception(impl->instance->instance);
				CAGE_LOG_THROW(e);
				CAGE_THROW_ERROR(Exception, "wasm function call failed");
			}
		}

		Holder<WasmFunctionInternal> wasmFindFunction(Holder<WasmInstance> &&instance, const String &name)
		{
			CAGE_ASSERT(instance);
			return systemMemory().createImpl<WasmFunctionInternal, WasmFunctionImpl>(std::move(instance), name);
		}
	}
}
