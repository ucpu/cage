#include <cmath> // max
#include <cstdarg> // va_list
#include <cstdio> // vsnprintf
#include <cstring> // strlen
#include <vector>

#include <wasm_export.h>

#include <cage-core/memoryUtils.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>
#include <cage-core/wasm.h>

extern "C"
{
	using namespace cage;

	int cage_wamr_vprintf(const char *format, std::va_list va)
	{
		String str;
		int res = std::vsnprintf(str.rawData(), str.MaxLength - 100, format, va);
		str.rawLength() = std::strlen(str.rawData());
		str = trim(str);
		CAGE_LOG(SeverityEnum::Info, "wasm-print", str);
		return res;
	}

	void cage_wamr_log(uint32 log_level, const char *file, int line, const char *format, ...)
	{
		String str;
		std::va_list va;
		va_start(va, format);
		std::vsnprintf(str.rawData(), str.MaxLength - 100, format, va);
		va_end(va);
		str.rawLength() = std::strlen(str.rawData());
		str = trim(str);
		SeverityEnum sev = SeverityEnum::Info;
		switch (log_level)
		{
			case WASM_LOG_LEVEL_FATAL:
				sev = SeverityEnum::Critical;
				break;
			case WASM_LOG_LEVEL_ERROR:
				sev = SeverityEnum::Error;
				break;
			case WASM_LOG_LEVEL_WARNING:
				sev = SeverityEnum::Warning;
				break;
			case WASM_LOG_LEVEL_DEBUG:
			case WASM_LOG_LEVEL_VERBOSE:
				sev = SeverityEnum::Info;
				break;
		}
		CAGE_LOG(sev, "wasm-log", str);
	}
}

namespace cage
{
	using namespace privat;

	namespace
	{
		void initialize()
		{
			static int dummy = []() -> int
			{
				if (!wasm_runtime_init())
					CAGE_THROW_ERROR(Exception, "failed to initialize wasm runtime");
				return 0;
			}();
			(void)dummy;
			thread_local int dummy2 = []() -> int
			{
				if (!wasm_runtime_init_thread_env())
					CAGE_THROW_ERROR(Exception, "failed to initialize wasm thread env");
				return 0;
			}();
			(void)dummy2;
		}

		WasmTypeEnum convert(wasm_valkind_t k)
		{
			switch (k)
			{
				case WASM_I32:
					return WasmTypeEnum::Int32;
				case WASM_I64:
					return WasmTypeEnum::Int64;
				case WASM_F32:
					return WasmTypeEnum::Float;
				case WASM_F64:
					return WasmTypeEnum::Double;
				default:
					return WasmTypeEnum::Unknown;
			}
		}

		uint32 countOfWasmVal(WasmTypeEnum v)
		{
			switch (v)
			{
				case WasmTypeEnum::Void:
				case WasmTypeEnum::Unknown:
					return 0;
				case WasmTypeEnum::Int32:
				case WasmTypeEnum::Float:
					return 1;
				case WasmTypeEnum::Int64:
				case WasmTypeEnum::Double:
					return 2;
			}
			return 0;
		}

		const char *toString(wasm_import_export_kind_t k)
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

		class WasmNativesImpl : public WasmNatives
		{
		public:
			std::vector<WasmNative> functions;

			WasmNativesImpl() { initialize(); }
		};

		class WasmModuleImpl : public WasmModule
		{
		public:
			Holder<PointerRange<char>> buffer;
			wasm_module_t module = nullptr;

			WasmModuleImpl(PointerRange<const char> buffer_, bool allowAot)
			{
				initialize();

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
				initialize();

				CAGE_ASSERT(module);
				String err;
				instance = wasm_runtime_instantiate(module->module, 1'000'000, 1'000'000, err.rawData(), err.MaxLength - 100);
				if (!instance)
				{
					err.rawLength() = std::strlen(err.rawData());
					CAGE_LOG_THROW(err);
					CAGE_THROW_ERROR(Exception, "failed instantiating wasm module");
				}
				exec = wasm_runtime_get_exec_env_singleton(instance);
				if (!exec)
					CAGE_THROW_ERROR(Exception, "failed creating wasm execution environment");
				wasm_runtime_set_user_data(exec, this);
			}

			~WasmInstanceImpl()
			{
				temporary = {};
				if (instance)
				{
					wasm_runtime_deinstantiate(instance);
					instance = nullptr;
				}
			}
		};

		class WasmFunctionImpl : public WasmFunctionInternal
		{
		public:
			const WasmName name;
			WasmInstanceImpl *const instance = nullptr;
			wasm_function_inst_t func = nullptr;

			WasmTypeEnum returns = WasmTypeEnum::Void;
			std::vector<WasmTypeEnum> params;
			std::vector<uint32> data;

			WasmFunctionImpl(WasmInstanceImpl *instance, const String &name) : name(name), instance(instance)
			{
				CAGE_ASSERT(wasm_runtime_thread_env_inited());

				func = wasm_runtime_lookup_function(instance->instance, name.c_str());
				if (!func)
				{
					CAGE_LOG_THROW(Stringizer() + "function: " + name);
					CAGE_THROW_ERROR(Exception, "wasm function not found");
				}

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
				std::vector<wasm_valkind_t> vt;
				vt.resize(pc);
				wasm_func_get_param_types(func, instance->instance, vt.data());
				uint32 sz = 0;
				for (wasm_valkind_t t : vt)
				{
					const WasmTypeEnum v = convert(t);
					params.push_back(v);
					sz += countOfWasmVal(v);
				}

				data.resize(std::max(sz, countOfWasmVal(returns)));
			}
		};

		template<uint32 A>
		void strCopyTruncate(detail::StringBase<A> &dst, const char *src)
		{
			uint32 len = std::strlen(src);
			if (len > A)
				len = A;
			dst = detail::StringBase<A>(PointerRange<const char>(src, src + len));
		}
	}

	namespace privat
	{
		const WasmTypelessDelegate *wasmCallbackAttachment(void *ctx)
		{
			return (const WasmTypelessDelegate *)wasm_runtime_get_function_attachment((wasm_exec_env_t)ctx);
		}

		WasmInstance *wasmCallbackInstance(void *ctx)
		{
			return (WasmInstance *)wasm_runtime_get_user_data((wasm_exec_env_t)ctx);
		}
	}

	void WasmNatives::commit(const WasmName &moduleName_)
	{
		WasmNativesImpl *impl = (WasmNativesImpl *)this;
		if (impl->functions.empty())
			return;

		struct Register : private Immovable
		{
			detail::StringBase<59> moduleName;
			std::vector<WasmNative> as;
			std::vector<NativeSymbol> bs;

			void commit()
			{
				bs.resize(as.size());
				for (uint32 i = 0; i < as.size(); i++)
				{
					const WasmNative &a = as[i];
					NativeSymbol &b = bs[i];
					b.symbol = a.name.c_str();
					b.signature = a.signature.c_str();
					b.func_ptr = a.funcPtr;
					b.attachment = (void *)&a.delegate;
				}
				if (!wasm_runtime_register_natives(moduleName.c_str(), bs.data(), bs.size()))
					CAGE_THROW_ERROR(Exception, "failed registering native functions for wasm");
			}
		};

		Holder<Register> pair = systemMemory().createHolder<Register>();
		pair->moduleName = moduleName_;
		std::swap(pair->as, impl->functions);

		static std::vector<Holder<Register>> natives;
		natives.push_back(std::move(pair));
		natives.back()->commit();
	}

	void WasmNatives::add_(WasmNative fnc)
	{
		WasmNativesImpl *impl = (WasmNativesImpl *)this;
		impl->functions.push_back(fnc);
	}

	Holder<WasmNatives> newWasmNatives()
	{
		return systemMemory().createImpl<WasmNatives, WasmNativesImpl>();
	}

	Holder<PointerRange<WasmSymbol>> WasmModule::symbolsImports() const
	{
		const WasmModuleImpl *impl = (const WasmModuleImpl *)this;
		const uint32 cnt = wasm_runtime_get_import_count(impl->module);
		PointerRangeHolder<WasmSymbol> arr;
		arr.reserve(cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			wasm_import_t t = {};
			wasm_runtime_get_import_type(impl->module, i, &t);
			WasmSymbol w;
			w.moduleName = t.module_name;
			strCopyTruncate(w.name, t.name);
			w.kind = toString(t.kind);
			w.linked = t.linked;
			arr.push_back(w);
		}
		return arr;
	}

	Holder<PointerRange<WasmSymbol>> WasmModule::symbolsExports() const
	{
		const WasmModuleImpl *impl = (const WasmModuleImpl *)this;
		const uint32 cnt = wasm_runtime_get_export_count(impl->module);
		PointerRangeHolder<WasmSymbol> arr;
		arr.reserve(cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			wasm_export_t t = {};
			wasm_runtime_get_export_type(impl->module, i, &t);
			WasmSymbol w;
			strCopyTruncate(w.name, t.name);
			w.kind = toString(t.kind);
			w.linked = true;
			arr.push_back(w);
		}
		return arr;
	}

	Holder<WasmModule> newWasmModule(PointerRange<const char> buffer, bool allowAot)
	{
		return systemMemory().createImpl<WasmModule, WasmModuleImpl>(buffer, allowAot);
	}

	WasmBuffer::WasmBuffer(WasmInstance *instance__) : instance_(instance__)
	{
		CAGE_ASSERT(instance__);
	}

	WasmBuffer::WasmBuffer(WasmBuffer &&other)
	{
		*this = std::move(other);
	}

	WasmBuffer &WasmBuffer::operator=(WasmBuffer &&other)
	{
		free();
		instance_ = other.instance_;
		std::swap(size_, other.size_);
		std::swap(capacity_, other.capacity_);
		std::swap(wasmPtr_, other.wasmPtr_);
		std::swap(owned_, other.owned_);
		return *this;
	}

	WasmBuffer::~WasmBuffer()
	{
		free();
	}

	void WasmBuffer::resize(uint32 s)
	{
		if (s > capacity_)
			CAGE_THROW_ERROR(OutOfMemory, "cannot resize wasm buffer over its capacity", s);
		size_ = s;
	}

	void WasmBuffer::free()
	{
		if (owned_)
		{
			CAGE_ASSERT(instance_);
			instance_->free(wasmPtr_);
		}
		size_ = capacity_ = 0;
		wasmPtr_ = 0;
		owned_ = false;
	}

	void WasmBuffer::write(PointerRange<const char> buff)
	{
		resize(buff.size());
		detail::memcpy(nativeAddr(), buff.data(), buff.size());
	}

	void *WasmBuffer::nativeAddr() const
	{
		CAGE_ASSERT(instance_);
		CAGE_ASSERT(capacity_ >= size_);
		return instance_->wasm2native(wasmPtr_, capacity_);
	}

	namespace privat
	{
		WasmInstance *WasmFunctionInternal::instance() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->instance;
		}

		WasmName WasmFunctionInternal::name() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->name;
		}

		WasmTypeEnum WasmFunctionInternal::returns() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->returns;
		}

		PointerRange<const WasmTypeEnum> WasmFunctionInternal::params() const
		{
			const WasmFunctionImpl *impl = (const WasmFunctionImpl *)this;
			return impl->params;
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
				CAGE_LOG_THROW(Stringizer() + "function: " + impl->name);
				CAGE_THROW_ERROR(Exception, "wasm function call failed");
			}
		}
	}

	Holder<WasmModule> WasmInstance::module() const
	{
		const WasmInstanceImpl *impl = (const WasmInstanceImpl *)this;
		return impl->module.share().cast<WasmModule>();
	}

	void *WasmInstance::wasm2native(uint32 address, uint32 size)
	{
		if (!address)
			return 0;
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		if (size && !wasm_runtime_validate_app_addr(impl->instance, address, size))
			CAGE_THROW_ERROR(Exception, "invalid address range in wasm2native");
		void *ret = wasm_runtime_addr_app_to_native(impl->instance, address);
		if (!ret)
			CAGE_THROW_ERROR(Exception, "invalid address in wasm2native");
		return ret;
	}

	uint32 WasmInstance::native2wasm(void *address, uint32 size)
	{
		if (!address)
			return 0;
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		if (size && !wasm_runtime_validate_native_addr(impl->instance, address, size))
			CAGE_THROW_ERROR(Exception, "invalid address range in native2wasm");
		const uint32 ret = numeric_cast<uint32>(wasm_runtime_addr_native_to_app(impl->instance, address));
		if (!ret)
			CAGE_THROW_ERROR(Exception, "invalid address in native2wasm");
		return ret;
	}

	uint32 WasmInstance::malloc(uint32 size)
	{
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		uint32 r = wasm_runtime_module_malloc(impl->instance, size, nullptr);
		if (r == 0)
			CAGE_THROW_ERROR(Exception, "failed wasm memory allocation");
		return r;
	}

	void WasmInstance::free(uint32 wasmAddr)
	{
		if (wasmAddr == 0)
			return;
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		wasm_runtime_module_free(impl->instance, wasmAddr);
	}

	uint32 WasmInstance::strLen(uint32 wasmAddr)
	{
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		uint64 a = 0, b = 0;
		if (!wasm_runtime_get_app_addr_range(impl->instance, wasmAddr, &a, &b))
			CAGE_THROW_ERROR(Exception, "invalid address in wasm strLen");
		const auto maxLen = b - wasmAddr;
		const char *ptr = (const char *)wasm2native(wasmAddr);
		for (uint32 i = 0; i < maxLen; i++)
			if (ptr[i] == 0)
				return i;
		CAGE_THROW_ERROR(Exception, "found no null in wasm strLen");
	}

	WasmBuffer WasmInstance::allocate(uint32 size, uint32 capacity, bool owned)
	{
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		WasmBuffer b(impl);
		if (capacity < size)
			capacity = size;
		b.owned_ = owned;
		b.wasmPtr_ = impl->malloc(capacity);
		b.capacity_ = capacity;
		b.size_ = size;
		return b;
	}

	WasmBuffer WasmInstance::adapt(uint32 wasmAddr, uint32 size, bool owned)
	{
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		WasmBuffer b(impl);
		b.owned_ = owned;
		b.wasmPtr_ = wasmAddr;
		b.capacity_ = b.size_ = size;
		return b;
	}

	Holder<WasmFunctionInternal> WasmInstance::function_(const WasmName &name)
	{
		WasmInstanceImpl *impl = (WasmInstanceImpl *)this;
		return systemMemory().createImpl<WasmFunctionInternal, WasmFunctionImpl>(impl, name);
	}

	Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> &&module)
	{
		return systemMemory().createImpl<WasmInstance, WasmInstanceImpl>(std::move(module));
	}
}
