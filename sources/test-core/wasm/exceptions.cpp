#include <stdint.h>
#include <print>

extern "C"
void callback_first(void);

extern "C"
void call_first(void)
{
	callback_first();
}

extern "C"
void print_msg(const char *ptr)
{
	std::printf("%s\n", ptr);
}

extern "C"
void throwing(void)
{
	throw "message in the exception";
}

extern "C"
int sanity_sum(int a, int b)
{
	return a + b;
}
