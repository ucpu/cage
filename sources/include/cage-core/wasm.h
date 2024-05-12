#ifndef guard_wasm_h_vhj1b548hfdt74h
#define guard_wasm_h_vhj1b548hfdt74h

#include <cage-core/core.h>

namespace cage
{
	class WasmInstance;

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

	class CAGE_CORE_API WasmInstance : private Immovable
	{
	public:
	};

	CAGE_CORE_API Holder<WasmModule> newWasmModule(PointerRange<const char> buffer, bool allowAot = false);
	CAGE_CORE_API Holder<WasmInstance> wasmInstantiate(Holder<WasmModule> module);
}

#endif // guard_wasm_h_vhj1b548hfdt74h
