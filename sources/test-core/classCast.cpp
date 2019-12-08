#include "main.h"

namespace
{
	class A { public: virtual ~A() {} };
	class B : public A {};
	class C : public A {};
}

void testClassCast()
{
	CAGE_TESTCASE("class_cast");

	holder<A> ah = detail::systemArena().createHolder<A>();
	holder<A> bh = detail::systemArena().createImpl<A, B>();
	A *a = ah.get();
	A *b = bh.get();
	CAGE_TEST(class_cast<A*>(a));
	CAGE_TEST(class_cast<A*>(b));
	CAGE_TEST(class_cast<B*>(b));
	CAGE_TEST_ASSERTED(class_cast<C*>(b));
	A *n = nullptr;
	CAGE_TEST(class_cast<A*>(n) == nullptr);
	CAGE_TEST(class_cast<B*>(n) == nullptr);
}
