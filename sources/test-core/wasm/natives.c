#include <stdint.h>

void cage_test_func1(void);
uint32_t cage_test_func2(uint32_t a, uint64_t b);
void cage_test_func3(const char *);

void cage_test_call1(void)
{
	cage_test_func1();
}

void cage_test_call2a(void)
{
	cage_test_func2(42, 13000000000123);
}

void cage_test_call2b(void)
{
	cage_test_func2(10, 11);
}

void cage_test_call3(void)
{
	cage_test_func3("hello");
}
