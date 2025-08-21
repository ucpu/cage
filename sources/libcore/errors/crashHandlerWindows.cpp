#ifdef CAGE_SYSTEM_WINDOWS

	#include "../windowsMinimumInclude.h"
	#include <DbgHelp.h>

	#pragma comment(lib, "DbgHelp.lib")
	#define EXCEPTION_CPP 0xE06D7363
	#define EXCEPTION_DOTNET 0xE0434352
	#define EXCEPTION_RENAME_THREAD 0x406D1388

	#include <cage-core/core.h>
	#include <cage-core/concurrent.h>

namespace cage
{
	void crashHandlerThreadInit()
	{
		// reserve safe memory on the stack for handling EXCEPTION_STACK_OVERFLOW
		ULONG v = 128 * 1024;
		SetThreadStackGuarantee(&v);
	}

	namespace
	{
		String exceptionCodeToString(uint64 code)
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
				case DBG_EXCEPTION_HANDLED:
					return "DBG_EXCEPTION_HANDLED";
				case DBG_CONTINUE:
					return "DBG_CONTINUE";
				case DBG_REPLY_LATER:
					return "DBG_REPLY_LATER";
				case DBG_TERMINATE_THREAD:
					return "DBG_TERMINATE_THREAD";
				case DBG_TERMINATE_PROCESS:
					return "DBG_TERMINATE_PROCESS";
				case DBG_CONTROL_C:
					return "DBG_CONTROL_C";
				case DBG_PRINTEXCEPTION_C:
					return "DBG_PRINTEXCEPTION_C";
				case DBG_RIPEXCEPTION:
					return "DBG_RIPEXCEPTION";
				case DBG_CONTROL_BREAK:
					return "DBG_CONTROL_BREAK";
				case DBG_COMMAND_EXCEPTION:
					return "DBG_COMMAND_EXCEPTION";
				case DBG_PRINTEXCEPTION_WIDE_C:
					return "DBG_PRINTEXCEPTION_WIDE_C";
				case DBG_EXCEPTION_NOT_HANDLED:
					return "DBG_EXCEPTION_NOT_HANDLED";
				case EXCEPTION_CPP:
					return "EXCEPTION_CPP";
				case EXCEPTION_DOTNET:
					return "EXCEPTION_DOTNET";
				case EXCEPTION_RENAME_THREAD:
					return "EXCEPTION_RENAME_THREAD";
				default:
					return Stringizer() + "unknown exception code: " + code;
			}
		}

		String consoleCodeToString(uint64 code)
		{
			switch (code)
			{
				case CTRL_C_EVENT:
					return "CTRL_C_EVENT";
				case CTRL_BREAK_EVENT:
					return "CTRL_BREAK_EVENT";
				case CTRL_CLOSE_EVENT:
					return "CTRL_CLOSE_EVENT";
				case CTRL_LOGOFF_EVENT:
					return "CTRL_LOGOFF_EVENT";
				case CTRL_SHUTDOWN_EVENT:
					return "CTRL_SHUTDOWN_EVENT";
				default:
					return Stringizer() + "unknown console code: " + code;
			}
		}

		bool ignoredExceptionCodes(uint64 code)
		{
			switch (code)
			{
				case DBG_PRINTEXCEPTION_C: // OutputDebugStringA
				case DBG_PRINTEXCEPTION_WIDE_C: // OutputDebugStringW
				case EXCEPTION_CPP: // regular c++ exception
				case EXCEPTION_RENAME_THREAD: // exception raised to rename current thread
					return true;
				default:
					return false;
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

		RecursiveMutex *handlerMutex()
		{
			static Holder<RecursiveMutex> *mut = new Holder<RecursiveMutex>(newRecursiveMutex()); // this leak is intentional
			return +*mut;
		}

		void commonHandler(PEXCEPTION_POINTERS ex)
		{
			if (IsDebuggerPresent())
				return;
			ScopeLock lock(handlerMutex());
			CAGE_LOG(SeverityEnum::Error, "crash-handler", Stringizer() + "crash handler: " + exceptionCodeToString(ex->ExceptionRecord->ExceptionCode));
			if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_DOTNET)
				return;
			CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "address: " + ex->ExceptionRecord->ExceptionAddress);
			if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ex->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				const uint64 mode = ex->ExceptionRecord->ExceptionInformation[0];
				const uint64 addr = ex->ExceptionRecord->ExceptionInformation[1];
				CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "violation: " + exceptionAccessModeToString(mode) + ", at address: " + addr);
			}
			else
			{
				for (uint32 i = 0; i < ex->ExceptionRecord->NumberParameters; i++)
					CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "parameter[" + i + "]: " + ex->ExceptionRecord->ExceptionInformation);
			}
			CAGE_LOG(SeverityEnum::Info, "crash-handler", Stringizer() + "in thread: " + currentThreadName());
			printStackTrace(ex);
		}

		LONG WINAPI vectoredHandler(PEXCEPTION_POINTERS ex)
		{
			if (!ignoredExceptionCodes(ex->ExceptionRecord->ExceptionCode))
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

		BOOL WINAPI consoleHandler(DWORD code)
		{
			CAGE_LOG(SeverityEnum::Error, "crash-handler", Stringizer() + "crash handler: " + consoleCodeToString(code));
			return FALSE; // let other handlers process it
		}

		struct SetupHandlers
		{
			SetupHandlers()
			{
				handlerMutex(); // allocate the mutex upfront
				crashHandlerThreadInit();
				if (!AddVectoredExceptionHandler(1, &vectoredHandler))
					CAGE_THROW_ERROR(SystemError, "AddVectoredExceptionHandler", GetLastError());
				if (!AddVectoredContinueHandler(1, &vectoredHandler))
					CAGE_THROW_ERROR(SystemError, "AddVectoredContinueHandler", GetLastError());
				previous = SetUnhandledExceptionFilter(&unhandledHandler);
				if (!SetConsoleCtrlHandler(&consoleHandler, TRUE))
					CAGE_THROW_ERROR(SystemError, "SetConsoleCtrlHandler", GetLastError());
			}
		} setupHandlers;
	}
}

#endif // CAGE_SYSTEM_WINDOWS
