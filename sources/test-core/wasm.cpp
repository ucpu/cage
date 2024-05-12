#include "main.h"

#include <cage-core/wasm.h>

PointerRange<const uint8> sums_wasm();
PointerRange<const uint8> strings_wasm();

namespace
{
	void printModuleDetails(WasmModule *module)
	{
		CAGE_LOG(SeverityEnum::Info, "wasm", "imports:");
		for (const auto &it : module->importInfo())
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "wasm", Stringizer() + it.moduleName + " " + it.kind + " " + it.name);
		CAGE_LOG(SeverityEnum::Info, "wasm", "exports:");
		for (const auto &it : module->exportInfo())
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "wasm", Stringizer() + it.moduleName + " " + it.kind + " " + it.name);
	}
}

void testWasm()
{
	CAGE_TESTCASE("wasm");

	{
		CAGE_TESTCASE("empty module");
		CAGE_TEST_THROWN(newWasmModule({}));
	}

	{
		CAGE_TESTCASE("sums");
		Holder<WasmModule> mod = newWasmModule(sums_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		// todo
	}

	{
		CAGE_TESTCASE("strings");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		// todo
	}
}
