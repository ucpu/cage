#include <cmath>

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

	bool test(float a, float b)
	{
		return std::abs(a - b) < 1e-5f;
	}

	bool test(double a, double b)
	{
		return std::abs(a - b) < 1e-12;
	}
}

void testWasm()
{
	CAGE_TESTCASE("wasm");

	{
		CAGE_TESTCASE("sums");
		Holder<WasmModule> mod = newWasmModule(sums_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		{
			WasmFunction<sint32(sint32, sint32)> f = wasmFindFunction(inst.share(), "sum_int32");
			CAGE_TEST(f(13, 42) == 13 + 42);
			CAGE_TEST(f(5, 7) == 5 + 7);
			CAGE_TEST(f(10, -5) == 10 - 5);
		}
		{
			WasmFunction<sint64(sint64, sint64)> f = wasmFindFunction(inst.share(), "sum_int64");
			CAGE_TEST(f(13, 42) == 13 + 42);
			CAGE_TEST(f(5'000'000'000'000, 7) == 5'000'000'000'000 + 7);
			CAGE_TEST(f(10, -5'000'000'000'000) == 10 - 5'000'000'000'000);
		}
		{
			WasmFunction<float(float, float)> f = wasmFindFunction(inst.share(), "sum_float");
			CAGE_TEST(test(f(13.1f, 42.6f), 13.1f + 42.6f));
			CAGE_TEST(test(f(0.5f, 0.7f), 0.5f + 0.7f));
			CAGE_TEST(test(f(0.5f, -0.7f), 0.5f - 0.7f));
			CAGE_TEST(test(f(3e7f, 4e5f), 3e7f + 4e5f));
			CAGE_TEST(test(f(3e7f, 4e-5f), 3e7f + 4e-5f));
		}
		{
			WasmFunction<double(double, double)> f = wasmFindFunction(inst.share(), "sum_double");
			CAGE_TEST(test(f(13.1, 42.6), 13.1 + 42.6));
			CAGE_TEST(test(f(0.5, 0.7), 0.5 + 0.7));
			CAGE_TEST(test(f(0.5, -0.7), 0.5 - 0.7));
			CAGE_TEST(test(f(3e7, 4e5), 3e7 + 4e5));
			CAGE_TEST(test(f(3e7, 4e-5), 3e7 + 4e-5));
		}
		{
			WasmFunction<sint32()> f = wasmFindFunction(inst.share(), "get_number");
			CAGE_TEST(f() == 42);
			CAGE_TEST(f() == 42);
			CAGE_TEST(f() == 42);
		}
		{
			WasmFunction<void(sint32)> f = wasmFindFunction(inst.share(), "give_number");
			f(42);
			f(42);
			f(42);
		}
		{
			CAGE_TEST_THROWN(wasmFindFunction(inst.share(), "sum_invalid"));
			CAGE_TEST_THROWN([&]() { WasmFunction<float(double, double)> f = wasmFindFunction(inst.share(), "sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction<double(float, double)> f = wasmFindFunction(inst.share(), "sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction<double(double, float)> f = wasmFindFunction(inst.share(), "sum_double"); }());
		}
	}

	{
		CAGE_TESTCASE("strings");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		// todo
	}

	{
		CAGE_TESTCASE("empty module");
		CAGE_TEST_THROWN(newWasmModule({}));
	}
}
