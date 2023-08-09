#ifdef CAGE_SYSTEM_WINDOWS
	#include <mutex>
	#include "incWin.h"
	#include <DbgHelp.h>
	#pragma comment(lib, "DbgHelp.lib")
	#define EXCEPTION_CPP_UNKNOWN 0xE06D7363
	#define EXCEPTION_RENAME_THREAD 0x406D1388

	#include <cage-core/core.h>

namespace cage
{
	namespace
	{
		StringPointer exceptionCodeToString(uint64 code)
		{
			switch (code)
			{
				case EXCEPTION_ACCESS_VIOLATION:
					return "EXCEPTION_ACCESS_VIOLATION";
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
					return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
				case EXCEPTION_BREAKPOINT:
					return "EXCEPTION_BREAKPOINT";
				case EXCEPTION_COLLIDED_UNWIND:
					return "EXCEPTION_COLLIDED_UNWIND";
				case EXCEPTION_DATATYPE_MISALIGNMENT:
					return "EXCEPTION_DATATYPE_MISALIGNMENT";
				case EXCEPTION_EXIT_UNWIND:
					return "EXCEPTION_EXIT_UNWIND";
				case EXCEPTION_FLT_DENORMAL_OPERAND:
					return "EXCEPTION_FLT_DENORMAL_OPERAND";
				case EXCEPTION_FLT_DIVIDE_BY_ZERO:
					return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
				case EXCEPTION_FLT_INEXACT_RESULT:
					return "EXCEPTION_FLT_INEXACT_RESULT";
				case EXCEPTION_FLT_INVALID_OPERATION:
					return "EXCEPTION_FLT_INVALID_OPERATION";
				case EXCEPTION_FLT_OVERFLOW:
					return "EXCEPTION_FLT_OVERFLOW";
				case EXCEPTION_FLT_STACK_CHECK:
					return "EXCEPTION_FLT_STACK_CHECK";
				case EXCEPTION_FLT_UNDERFLOW:
					return "EXCEPTION_FLT_UNDERFLOW";
				case EXCEPTION_GUARD_PAGE:
					return "EXCEPTION_GUARD_PAGE";
				case EXCEPTION_ILLEGAL_INSTRUCTION:
					return "EXCEPTION_ILLEGAL_INSTRUCTION";
				case EXCEPTION_INT_DIVIDE_BY_ZERO:
					return "EXCEPTION_INT_DIVIDE_BY_ZERO";
				case EXCEPTION_INT_OVERFLOW:
					return "EXCEPTION_INT_OVERFLOW";
				case EXCEPTION_INVALID_DISPOSITION:
					return "EXCEPTION_INVALID_DISPOSITION";
				case EXCEPTION_INVALID_HANDLE:
					return "EXCEPTION_INVALID_HANDLE";
				case EXCEPTION_IN_PAGE_ERROR:
					return "EXCEPTION_IN_PAGE_ERROR";
				case EXCEPTION_NESTED_CALL:
					return "EXCEPTION_NESTED_CALL";
				case EXCEPTION_NONCONTINUABLE_EXCEPTION:
					return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
				//case EXCEPTION_POSSIBLE_DEADLOCK:
				//	return "EXCEPTION_POSSIBLE_DEADLOCK";
				case EXCEPTION_PRIV_INSTRUCTION:
					return "EXCEPTION_PRIV_INSTRUCTION";
				case EXCEPTION_SINGLE_STEP:
					return "EXCEPTION_SINGLE_STEP";
				case EXCEPTION_STACK_INVALID:
					return "EXCEPTION_STACK_INVALID";
				case EXCEPTION_STACK_OVERFLOW:
					return "EXCEPTION_STACK_OVERFLOW";
				case EXCEPTION_TARGET_UNWIND:
					return "EXCEPTION_TARGET_UNWIND";
				case EXCEPTION_UNWINDING:
					return "EXCEPTION_UNWINDING";
				case DBG_CONTROL_C:
					return "DBG_CONTROL_C";
				case EXCEPTION_CPP_UNKNOWN:
					return "EXCEPTION_CPP_UNKNOWN";
				case EXCEPTION_RENAME_THREAD:
					return "EXCEPTION_RENAME_THREAD";
				default:
					return "UNKNOWN_EXCEPTION_CODE";
			}
		}

		StringPointer exceptionAccessModeToString(uint64 mode)
		{
			switch (mode)
			{
				case EXCEPTION_READ_FAULT:
					return "read access";
				case EXCEPTION_WRITE_FAULT:
					return "write access";
				case EXCEPTION_EXECUTE_FAULT:
					return "data execution prevention";
				default:
					return "unknown access mode";
			}
		}

		void printStackTrace(PEXCEPTION_POINTERS ex)
		{
			const HANDLE process = GetCurrentProcess();
			if (SymInitialize(process, nullptr, true))
			{
				const HANDLE thread = GetCurrentThread();
				CONTEXT context = *ex->ContextRecord;
				STACKFRAME64 stackFrame = {};
	#if defined(_M_IX86)
				static constexpr DWORD machine = IMAGE_FILE_MACHINE_I386;
				stackFrame.AddrPC.Offset = context.Eip;
				stackFrame.AddrPC.Mode = AddrModeFlat;
				stackFrame.AddrFrame.Offset = context.Ebp;
				stackFrame.AddrFrame.Mode = AddrModeFlat;
				stackFrame.AddrStack.Offset = context.Esp;
				stackFrame.AddrStack.Mode = AddrModeFlat;
	#elif defined(_M_X64)
				static constexpr DWORD machine = IMAGE_FILE_MACHINE_AMD64;
				stackFrame.AddrPC.Offset = context.Rip;
				stackFrame.AddrPC.Mode = AddrModeFlat;
				stackFrame.AddrFrame.Offset = context.Rbp;
				stackFrame.AddrFrame.Mode = AddrModeFlat;
				stackFrame.AddrStack.Offset = context.Rsp;
				stackFrame.AddrStack.Mode = AddrModeFlat;
	#else
		#error "processor type not supported"
	#endif
				CAGE_LOG(SeverityEnum::Info, "crash-handler", "stack trace:");
				char symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_PATH] = {};
				PIMAGEHLP_SYMBOL64 symbol = (PIMAGEHLP_SYMBOL64)symbolBuffer;
				symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
				symbol->MaxNameLength = MAX_PATH;
				IMAGEHLP_LINE64 line = {};
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				DWORD displacement;
				uint32 iteration = 0;
				while (StackWalk64(machine, process, thread, &stackFrame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
				{
					Stringizer str;
					if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &displacement, &line))
						str + line.FileName + "(" + (uint64)line.LineNumber + ")";
					if (SymGetSymFromAddr64(process, stackFrame.AddrPC.Offset, nullptr, symbol))
					{
						if (!str.value.empty())
							str + ": ";
						str + symbol->Name;
					}
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "crash-handler", str.value.empty() ? String("unknown-frame") : str);
					if (++iteration == 50)
						break;
				}
				SymCleanup(process);
			}
			else
			{
				CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "cannot print stack trace - SymInitialize failed: " + (uint64)GetLastError());
			}
		}

		void commonHandler(PEXCEPTION_POINTERS ex)
		{
			if (IsDebuggerPresent())
				return;
			static std::mutex mutex;
			std::scoped_lock lock(mutex);
			CAGE_LOG(SeverityEnum::Error, "crash-handler", Stringizer() + "crash handler: " + exceptionCodeToString(ex->ExceptionRecord->ExceptionCode));
			if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ex->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				const uint64 mode = ex->ExceptionRecord->ExceptionInformation[0];
				const uint64 addr = ex->ExceptionRecord->ExceptionInformation[1];
				CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "violation: " + exceptionAccessModeToString(mode) + ", at address: " + addr);
			}
			printStackTrace(ex);
		}

		LONG WINAPI vectoredHandler(PEXCEPTION_POINTERS ex)
		{
			if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_CPP_UNKNOWN || ex->ExceptionRecord->ExceptionCode == EXCEPTION_RENAME_THREAD)
				return EXCEPTION_CONTINUE_SEARCH;
			commonHandler(ex);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		LONG WINAPI continueHandler(PEXCEPTION_POINTERS ex)
		{
			if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_CPP_UNKNOWN || ex->ExceptionRecord->ExceptionCode == EXCEPTION_RENAME_THREAD)
				return EXCEPTION_CONTINUE_SEARCH;
			commonHandler(ex);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		LPTOP_LEVEL_EXCEPTION_FILTER previous = nullptr;

		LONG WINAPI unhandledHandler(PEXCEPTION_POINTERS ex)
		{
			commonHandler(ex);
			if (previous)
				return previous(ex);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		struct SetupHandlers
		{
			SetupHandlers()
			{
				AddVectoredExceptionHandler(1, &vectoredHandler);
				AddVectoredContinueHandler(1, &continueHandler);
				previous = SetUnhandledExceptionFilter(&unhandledHandler);
			}
		} setupHandlers;
	}
}

#endif // CAGE_SYSTEM_WINDOWS
