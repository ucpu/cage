#include <cmath>
#include <cstring>

#include "main.h"

#include <cage-core/wasm.h>

PointerRange<const uint8> sums_wasm();
PointerRange<const uint8> strings_wasm();
PointerRange<const uint8> natives_wasm();

namespace
{
	void printModuleDetails(WasmModule *module)
	{
		CAGE_LOG(SeverityEnum::Info, "wasm", "imports:");
		for (const auto &it : module->importsInfo())
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "wasm", Stringizer() + it.moduleName + " " + it.kind + " " + it.name + (it.linked ? "" : " not-linked"));
		CAGE_LOG(SeverityEnum::Info, "wasm", "exports:");
		for (const auto &it : module->exportsInfo())
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "wasm", Stringizer() + it.kind + " " + it.name);
	}

	bool test(float a, float b)
	{
		return std::abs(a - b) < 1e-5f;
	}

	bool test(double a, double b)
	{
		return std::abs(a - b) < 1e-12;
	}

	bool test(const void *ptr, const String &s)
	{
		if (ptr)
			return String((const char *)ptr) == s;
		return s == "";
	}
}

void testWasm()
{
	CAGE_TESTCASE("wasm");

	{
		CAGE_TESTCASE("simple functions");
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
			f(43);
			f(44);
		}
		{
			CAGE_TESTCASE("invalid functions");
			CAGE_TEST_THROWN(wasmFindFunction(inst.share(), "sum_invalid"));
			CAGE_TEST_THROWN([&]() { WasmFunction<float(double, double)> f = wasmFindFunction(inst.share(), "sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction<double(float, double)> f = wasmFindFunction(inst.share(), "sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction<double(double, float)> f = wasmFindFunction(inst.share(), "sum_double"); }());
		}
	}

	{
		CAGE_TESTCASE("functions with buffers");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction<void()> clear_string = wasmFindFunction(inst.share(), "clear_string");
		WasmFunction<void(sint32)> generate_string = wasmFindFunction(inst.share(), "generate_string");
		WasmFunction<void(sint32 addr, sint32 size)> set_string = wasmFindFunction(inst.share(), "set_string");
		WasmFunction<sint32()> get_string = wasmFindFunction(inst.share(), "get_string");
		WasmFunction<void()> print_string = wasmFindFunction(inst.share(), "print_string");
		generate_string(20);
		print_string();
		CAGE_TEST(test(inst->wasm2native(get_string()), "ABCDEFGHIJKLMNOPQRST"));
		{
			auto buf = wasmAllocate(inst.share(), 30);
			std::strcpy((char *)buf->nativeAddr(), "hello");
			set_string(buf->wasmAddr(), buf->size());
		}
		CAGE_TEST(test(inst->wasm2native(get_string()), "hello"));
		clear_string();
		CAGE_TEST(test(inst->wasm2native(get_string()), ""));
		{
			CAGE_TESTCASE("invalid addresses");
			CAGE_TEST_THROWN(inst->native2wasm(&mod));
			CAGE_TEST_THROWN(inst->wasm2native(1'000'000'000));
		}
	}

	{
		CAGE_TESTCASE("multiple instantiations of same module");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		Holder<WasmInstance> inst1 = wasmInstantiate(mod.share());
		Holder<WasmInstance> inst2 = wasmInstantiate(mod.share());
		WasmFunction<void(sint32)> gen1 = wasmFindFunction(inst1.share(), "generate_string");
		WasmFunction<void(sint32)> gen2 = wasmFindFunction(inst2.share(), "generate_string");
		WasmFunction<sint32()> get1 = wasmFindFunction(inst1.share(), "get_string");
		WasmFunction<sint32()> get2 = wasmFindFunction(inst2.share(), "get_string");
		CAGE_TEST(test(inst1->wasm2native(get1()), ""));
		CAGE_TEST(test(inst2->wasm2native(get2()), ""));
		gen1(5);
		gen2(10);
		CAGE_TEST(test(inst1->wasm2native(get1()), "ABCDE"));
		CAGE_TEST(test(inst2->wasm2native(get2()), "ABCDEFGHIJ"));
	}

	{
		CAGE_TESTCASE("unlinked native functions");
		Holder<WasmModule> mod = newWasmModule(natives_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction<void()> call1 = wasmFindFunction(inst.share(), "cage_test_call1");
		CAGE_TEST_THROWN(call1());
	}

	{
		CAGE_TESTCASE("linked native functions");
		uint32 fnc1cnt = 0, fnc2cnt = 0, fnc3cnt = 0;
		{
			Holder<WasmNatives> nats = newWasmNatives();
			nats->add(Delegate<void()>([&]() { fnc1cnt++; }), "cage_test_func1");
			nats->add(Delegate<uint32(uint32, uint64)>(
						  [&](uint32 a, uint64 b)
						  {
							  fnc2cnt += a == 42 && b == 13'000'000'000'123;
							  return a;
						  }),
				"cage_test_func2");
			nats->add(Delegate<void(WasmInstance *, uint32)>(
						  [&](WasmInstance *inst, uint32 addr)
						  {
							  CAGE_TEST(inst);
							  CAGE_TEST(test(inst->wasm2native(addr), "hello"));
							  fnc3cnt++;
						  }),
				"cage_test_func3");
			nats->commit();
		}
		Holder<WasmModule> mod = newWasmModule(natives_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction<void()> call1 = wasmFindFunction(inst.share(), "cage_test_call1");
		WasmFunction<void()> call2a = wasmFindFunction(inst.share(), "cage_test_call2a"); // calls cage_test_func2 with valid numbers
		WasmFunction<void()> call2b = wasmFindFunction(inst.share(), "cage_test_call2b"); // calls cage_test_func2 with different numbers
		WasmFunction<void()> call3 = wasmFindFunction(inst.share(), "cage_test_call3");
		CAGE_TEST(fnc1cnt == 0);
		CAGE_TEST(fnc2cnt == 0);
		CAGE_TEST(fnc3cnt == 0);
		call1();
		CAGE_TEST(fnc1cnt == 1);
		call1();
		CAGE_TEST(fnc1cnt == 2);
		call1();
		CAGE_TEST(fnc1cnt == 3);
		CAGE_TEST(fnc2cnt == 0);
		call2a();
		CAGE_TEST(fnc1cnt == 3);
		CAGE_TEST(fnc2cnt == 1);
		call2b();
		CAGE_TEST(fnc1cnt == 3);
		CAGE_TEST(fnc2cnt == 1);
		call3();
		CAGE_TEST(fnc3cnt == 1);
	}

	{
		CAGE_TESTCASE("empty module");
		CAGE_TEST_THROWN(newWasmModule({}));
	}
}
