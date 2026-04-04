#ifdef CAGE_SYSTEM_MAC

	#include <dlfcn.h>
	#include <mach/exception_types.h>
	#include <mach/mach.h>
	#include <mach/task.h>
	#include <mach/thread_act.h>
	#include <pthread.h>
	#include <unistd.h>

extern "C"
{
	#include "mach_excServer.h"
}

	#include <cage-core/core.h>

namespace cage
{
	namespace privat
	{
		void crashHandlerSafeWrite(const String &s);
	}

	namespace
	{
		const char *excToStr(exception_type_t exc)
		{
			switch (exc)
			{
				case EXC_BAD_ACCESS:
					return "EXC_BAD_ACCESS";
				case EXC_ARITHMETIC:
					return "EXC_ARITHMETIC";
				case EXC_BAD_INSTRUCTION:
					return "EXC_ILLEGAL_INSTRUCTION";
				case EXC_BREAKPOINT:
					return "EXC_BREAKPOINT";
				case EXC_CRASH:
					return "EXC_CRASH";
				case EXC_RESOURCE:
					return "EXC_RESOURCE";
				case EXC_GUARD:
					return "EXC_GUARD";
				default:
					return "UNKNOWN";
			}
		}

		void writeThreadInfo(mach_port_t thread)
		{
			const pthread_t pt = pthread_from_mach_thread_np(thread);
			if (!pt)
			{
				privat::crashHandlerSafeWrite("thread id: unknown\n");
				return;
			}

			uint64 tid = 0;
			pthread_threadid_np(pt, &tid);
			privat::crashHandlerSafeWrite(Stringizer() + "thread id: " + tid + "\n");

			// best-effort thread name (unsafe if crashing)
			char name[64] = {};
			pthread_getname_np(pt, name, sizeof(name));
			if (name[0])
			{
				name[sizeof(name) - 1] = 0;
				privat::crashHandlerSafeWrite(Stringizer() + "thread name: " + name + "\n");
			}
		}

		std::pair<uint64, uint64> writeRegisters(mach_port_t thread)
		{
	#if defined(__x86_64__)
			x86_thread_state64_t state;
			mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;
			if (thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&state, &count) == KERN_SUCCESS)
			{
				privat::crashHandlerSafeWrite(Stringizer() + "rip: " + (void *)state.__rip + "\n");
				privat::crashHandlerSafeWrite(Stringizer() + "rsp: " + (void *)state.__rsp + "\n");
				privat::crashHandlerSafeWrite(Stringizer() + "rbp: " + (void *)state.__rbp + "\n");
				return { (uint64)state.__rbp, (uint64)state.__rip };
			}
	#elif defined(__arm64__)
			arm_thread_state64_t state;
			mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
			if (thread_get_state(thread, ARM_THREAD_STATE64, (thread_state_t)&state, &count) == KERN_SUCCESS)
			{
				privat::crashHandlerSafeWrite(Stringizer() + "pc: " + (void *)state.__pc + "\n");
				privat::crashHandlerSafeWrite(Stringizer() + "sp: " + (void *)state.__sp + "\n");
				privat::crashHandlerSafeWrite(Stringizer() + "fp: " + (void *)state.__fp + "\n");
				return { (uint64)state.__fp, (uint64)state.__pc };
			}
	#else
		#error "unknwon cpu architecture"
	#endif
			return {};
		}

		void printStack(uint64 fp, uint64 pc, mach_port_t thread)
		{
			privat::crashHandlerSafeWrite("stack trace:\n");

			const auto &printFrame = [](uint64 addr)
			{
				Dl_info info = {};
				if (dladdr((void *)addr, &info) && info.dli_sname)
					privat::crashHandlerSafeWrite(Stringizer() + info.dli_sname + " + " + (uint64)((char *)addr - (char *)info.dli_saddr) + "\n");
				else
					privat::crashHandlerSafeWrite(Stringizer() + (void *)addr + "\n");
			};

			if (pc)
				printFrame(pc);

			const pthread_t pt = pthread_from_mach_thread_np(thread);
			if (!pt)
			{
				privat::crashHandlerSafeWrite("unknown thread id\n");
				return;
			}

			const uint64 top = (uint64)pthread_get_stackaddr_np(pt);
			const uint64 bottom = top - (uint64)pthread_get_stacksize_np(pt);

			for (uint32 i = 0; i < 100; i++)
			{
				if (fp < bottom || fp + 2 * sizeof(uint64) > top)
					break;
				if (fp % alignof(void *) != 0)
					break;
				const uint64_t *frame = (uint64_t *)fp;
				if (frame[1] == 0)
					break;
				printFrame(frame[1]);
				fp = frame[0];
			}
			privat::crashHandlerSafeWrite("\n");
		}
	}
}

extern "C"
{
	kern_return_t catch_mach_exception_raise(mach_port_t exception_port, mach_port_t thread, mach_port_t task, exception_type_t exception, exception_data_t code, mach_msg_type_number_t code_count)
	{
		using namespace cage;
		privat::crashHandlerSafeWrite(Stringizer() + "mach exception: " + excToStr(exception) + " (" + exception + ")\n");
		if (code_count > 0)
			privat::crashHandlerSafeWrite(Stringizer() + "code[0]: " + code[0] + "\n");
		if (code_count > 1)
			privat::crashHandlerSafeWrite(Stringizer() + "code[1]: " + code[1] + "\n");
		writeThreadInfo(thread);
		const auto regs = writeRegisters(thread);
		printStack(regs.first, regs.second, thread);
		privat::crashHandlerSafeWrite("mach exception handler done\n");
		// let the system continue propagating the exception
		return KERN_FAILURE;
	}

	kern_return_t catch_mach_exception_raise_state(mach_port_t, exception_type_t, const exception_data_t, mach_msg_type_number_t, int *, const thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *)
	{
		return KERN_NOT_SUPPORTED;
	}

	kern_return_t catch_mach_exception_raise_state_identity(mach_port_t, mach_port_t, mach_port_t, exception_type_t, exception_data_t, mach_msg_type_number_t, int *, const thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *)
	{
		return KERN_NOT_SUPPORTED;
	}

	extern boolean_t mach_exc_server(mach_msg_header_t *msg, mach_msg_header_t *reply);
} // extern "C"

namespace cage
{
	namespace
	{
		mach_port_t exceptionPort = MACH_PORT_NULL;

		void *machMsgServerEntry(void *)
		{
			while (true)
			{
				// this will call the catch_mach_exception_* functions
				static constexpr std::size_t MachMsgSize = 16'384;
				kern_return_t kr = mach_msg_server(mach_exc_server, MachMsgSize, exceptionPort, 0);
				if (kr != KERN_SUCCESS)
				{
					privat::crashHandlerSafeWrite("mach_msg_server failure, stopping the exception-handling thread\n");
					break;
				}
			}
			return nullptr;
		}

		struct SetupHandlersMac
		{
			SetupHandlersMac()
			{
				// allocate port
				kern_return_t kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exceptionPort);
				if (kr != KERN_SUCCESS)
				{
					CAGE_LOG(SeverityEnum::Warning, "crash-handler", "failed to allocate exception handling port");
					return;
				}

				// allow sending to it
				mach_port_insert_right(mach_task_self(), exceptionPort, exceptionPort, MACH_MSG_TYPE_MAKE_SEND);

				// catch common fatal exceptions
				static constexpr exception_mask_t mask = EXC_MASK_BAD_ACCESS | EXC_MASK_ARITHMETIC | EXC_MASK_BAD_INSTRUCTION | EXC_MASK_BREAKPOINT | EXC_MASK_CRASH | EXC_MASK_RESOURCE | EXC_MASK_GUARD;
				kr = task_set_exception_ports(mach_task_self(), mask, exceptionPort, EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, THREAD_STATE_NONE);
				if (kr != KERN_SUCCESS)
				{
					CAGE_LOG(SeverityEnum::Warning, "crash-handler", "failed to setup exception handling mask");
					return;
				}

				// start handler thread
				pthread_t t;
				pthread_create(&t, nullptr, machMsgServerEntry, nullptr);
				pthread_detach(t);
			}
		} setupHandlersMac;
	}
}

#endif // CAGE_SYSTEM_MAC
