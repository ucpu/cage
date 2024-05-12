#include <cstring> // std::strlen

#include <wasm_export.h>

#include <cage-core/pointerRangeHolder.h>
#include <cage-core/wasm.h>

extern "C"
{
	using namespace cage;

	int cage_wamr_vprintf(const char *format, va_list ap)
	{
		String str;
		int res = vsnprintf(str.rawData(), str.MaxLength - 100, format, ap);
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
			Holder<WasmModule> module;
			wasm_module_inst_t instance = nullptr;

			WasmInstanceImpl(Holder<WasmModule> &&module_) : module(std::move(module_))
			{
				WasmModuleImpl *impl = (WasmModuleImpl *)+module;
				String err;
				instance = wasm_runtime_instantiate(impl->module, 1'000'000, 1'000'000, err.rawData(), err.MaxLength - 100);
				if (!instance)
				{
					err.rawLength() = std::strlen(err.rawData());
					CAGE_LOG_CONTINUE(SeverityEnum::Note, "wasm", err);
					CAGE_THROW_ERROR(Exception, "failed instantiating wasm module");
				}
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

	Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> module)
	{
		CAGE_ASSERT(module);
		return systemMemory().createImpl<WasmInstance, WasmInstanceImpl>(std::move(module));
	}
}
