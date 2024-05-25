#include <cmath>

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
		for (const auto &it : module->symbolsImports())
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "wasm", Stringizer() + it.moduleName + " " + it.kind + " " + it.name + (it.linked ? "" : " not-linked"));
		CAGE_LOG(SeverityEnum::Info, "wasm", "exports:");
		for (const auto &it : module->symbolsExports())
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

	bool testStr(WasmInstance *inst, uint32 wasmAddr, const String &s)
	{
		if (!wasmAddr)
			return s == "";
		WasmBuffer buf = inst->adapt(wasmAddr, inst->strLen(wasmAddr) + 1);
		return buf.read() == s;
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
			WasmFunction f = inst->function<sint32(sint32, sint32)>("sum_int32");
			CAGE_TEST(f(13, 42) == 13 + 42);
			CAGE_TEST(f(5, 7) == 5 + 7);
			CAGE_TEST(f(10, -5) == 10 - 5);
		}
		{
			WasmFunction f = inst->function<sint64(sint64, sint64)>("sum_int64");
			CAGE_TEST(f(13, 42) == 13 + 42);
			CAGE_TEST(f(5'000'000'000'000, 7) == 5'000'000'000'000 + 7);
			CAGE_TEST(f(10, -5'000'000'000'000) == 10 - 5'000'000'000'000);
		}
		{
			WasmFunction f = inst->function<float(float, float)>("sum_float");
			CAGE_TEST(test(f(13.1f, 42.6f), 13.1f + 42.6f));
			CAGE_TEST(test(f(0.5f, 0.7f), 0.5f + 0.7f));
			CAGE_TEST(test(f(0.5f, -0.7f), 0.5f - 0.7f));
			CAGE_TEST(test(f(3e7f, 4e5f), 3e7f + 4e5f));
			CAGE_TEST(test(f(3e7f, 4e-5f), 3e7f + 4e-5f));
		}
		{
			WasmFunction f = inst->function<double(double, double)>("sum_double");
			CAGE_TEST(test(f(13.1, 42.6), 13.1 + 42.6));
			CAGE_TEST(test(f(0.5, 0.7), 0.5 + 0.7));
			CAGE_TEST(test(f(0.5, -0.7), 0.5 - 0.7));
			CAGE_TEST(test(f(3e7, 4e5), 3e7 + 4e5));
			CAGE_TEST(test(f(3e7, 4e-5), 3e7 + 4e-5));
		}
		{
			WasmFunction f = inst->function<sint32()>("get_number");
			CAGE_TEST(f() == 42);
			CAGE_TEST(f() == 42);
			CAGE_TEST(f() == 42);
		}
		{
			WasmFunction f = inst->function<void(sint32)>("give_number");
			f(42);
			f(43);
			f(44);
		}
		{
			WasmFunction f = inst->function<sint32(sint32, sint32)>("div_int32");
			CAGE_TEST(f(19, 5) == 19 / 5);
			CAGE_TEST(f(20, 5) == 20 / 5);
			CAGE_TEST(f(21, 5) == 21 / 5);
			CAGE_TEST(f(-20, 5) == -20 / 5);
			CAGE_TEST(f(20, -5) == 20 / -5);
			CAGE_TEST(f(-20, -5) == -20 / -5);
		}
		{
			CAGE_TESTCASE("invalid functions");
			CAGE_TEST_THROWN(inst->function<float(float, float)>("sum_invalid"));
			CAGE_TEST_THROWN([&]() { WasmFunction f = inst->function<float(double, double)>("sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction f = inst->function<double(float, double)>("sum_double"); }());
			CAGE_TEST_THROWN([&]() { WasmFunction f = inst->function<double(double, float)>("sum_double"); }());
		}
		{
			CAGE_TESTCASE("division by zero inside wasm");
			WasmFunction f = inst->function<sint32(sint32, sint32)>("div_int32");
			CAGE_TEST_THROWN(f(10, 0));
		}
	}

	{
		CAGE_TESTCASE("functions with buffers");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction clear_string = inst->function<void()>("clear_string");
		WasmFunction generate_string = inst->function<void(uint32)>("generate_string");
		WasmFunction set_string = inst->function<void(uint32 addr, uint32 size)>("set_string");
		WasmFunction get_string = inst->function<uint32()>("get_string");
		WasmFunction get_length = inst->function<uint32()>("get_length");
		WasmFunction print_string = inst->function<void()>("print_string");
		generate_string(20);
		print_string();
		CAGE_TEST(get_length() == 20);
		CAGE_TEST(testStr(+inst, get_string(), "ABCDEFGHIJKLMNOPQRST"));
		CAGE_TEST(inst->strLen(get_string() + 10) == 10);
		CAGE_TEST(inst->strLen(get_string() + 20) == 0);
		{
			auto buf = inst->allocate(30);
			buf.write("hello");
			set_string(buf.wasmAddr(), buf.size());
		}
		CAGE_TEST(get_length() == 5);
		CAGE_TEST(testStr(+inst, get_string(), "hello"));
		clear_string();
		CAGE_TEST(testStr(+inst, get_string(), ""));
		{
			CAGE_TESTCASE("invalid addresses");
			CAGE_TEST_THROWN(inst->native2wasm(&mod));
			CAGE_TEST_THROWN(inst->wasm2native(1'000'000'000));
			CAGE_TEST_THROWN(inst->strLen(1'000'000'000));
		}
		{
			CAGE_TESTCASE("invalid memory access inside wasm");
			CAGE_TEST_THROWN(set_string(1'000'000'000, 32));
		}
	}

	{
		CAGE_TESTCASE("multiple instantiations of same module");
		Holder<WasmModule> mod = newWasmModule(strings_wasm().cast<const char>());
		Holder<WasmInstance> inst1 = wasmInstantiate(mod.share());
		Holder<WasmInstance> inst2 = wasmInstantiate(mod.share());
		WasmFunction gen1 = inst1->function<void(uint32)>("generate_string");
		WasmFunction gen2 = inst2->function<void(uint32)>("generate_string");
		WasmFunction get1 = inst1->function<uint32()>("get_string");
		WasmFunction get2 = inst2->function<uint32()>("get_string");
		CAGE_TEST(testStr(+inst1, get1(), ""));
		CAGE_TEST(testStr(+inst2, get2(), ""));
		gen1(5);
		gen2(10);
		CAGE_TEST(testStr(+inst1, get1(), "ABCDE"));
		CAGE_TEST(testStr(+inst2, get2(), "ABCDEFGHIJ"));
	}

	{
		CAGE_TESTCASE("unlinked native functions");
		Holder<WasmModule> mod = newWasmModule(natives_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction call1 = inst->function<void()>("cage_test_call1");
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
							  CAGE_TEST(testStr(inst, addr, "hello"));
							  fnc3cnt++;
						  }),
				"cage_test_func3");
			nats->commit();
		}
		Holder<WasmModule> mod = newWasmModule(natives_wasm().cast<const char>());
		printModuleDetails(+mod);
		Holder<WasmInstance> inst = wasmInstantiate(mod.share());
		WasmFunction call1 = inst->function<void()>("cage_test_call1");
		WasmFunction call2a = inst->function<void()>("cage_test_call2a"); // calls cage_test_func2 with valid numbers
		WasmFunction call2b = inst->function<void()>("cage_test_call2b"); // calls cage_test_func2 with different numbers
		WasmFunction call3 = inst->function<void()>("cage_test_call3");
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
