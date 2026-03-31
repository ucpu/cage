#include <cstdio> // std::printf
#include <cstdlib> // std::abort
#include <exception> // std::terminate

#include <cage-core/logger.h>

using namespace cage;

void crashWriteToNull()
{
	char *ptr = nullptr;
	*ptr = 42;
}

void crashExecuteNull()
{
	using Func = void (*)();
	Func func = nullptr;
	func();
}

void crashCallTerminate()
{
	std::terminate();
}

void crashCallAbort()
{
	std::abort();
}

void crashStackOverflow()
{
	volatile char buffer[1024]; // prevent tail-call optimization
	detail::memset((void *)buffer, 0, sizeof(buffer));
	if (!buffer[42]) // supress warning
		crashStackOverflow();
}

void crashDivideByZero()
{
	volatile int a = 1;
	volatile int b = 0;
	volatile int c = a / b;
	(void)c;
}

void crashInvalidParameter()
{
	std::printf(nullptr); // invalid format string
}

void crashDoubleFree()
{
	int *p = new int;
	delete p;
	delete p;
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		if (argc != 2)
			CAGE_THROW_ERROR(Exception, "need crash type name");
		const String name = args[1];
		if (name == "no-crash")
			return 0;
		if (name == "write-to-null")
		{
			crashWriteToNull();
			return 0;
		}
		if (name == "execute-null")
		{
			crashExecuteNull();
			return 0;
		}
		if (name == "terminate")
		{
			crashCallTerminate();
			return 0;
		}
		if (name == "abort")
		{
			crashCallAbort();
			return 0;
		}
		if (name == "stack-overflow")
		{
			crashStackOverflow();
			return 0;
		}
		if (name == "divide-by-zero")
		{
			crashDivideByZero();
			return 0;
		}
		if (name == "invalid-parameter")
		{
			crashInvalidParameter();
			return 0;
		}
		if (name == "double-free")
		{
			crashDoubleFree();
			return 0;
		}
		CAGE_THROW_ERROR(Exception, "unknown crash name");
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
