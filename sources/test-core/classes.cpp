#include "main.h"

namespace
{
	class Base
	{
	public:
		int base;
		Base() : base(1) {}
		virtual ~Base() {}
	};

	class Derived : public Base
	{
	public:
		int derived;
		Derived() : derived(2) {}
	};

	class OtherDerived : public Base
	{
	public:
		int otherDerived;
		OtherDerived() : otherDerived(3) {}
	};

	class VirtualDerived : public virtual Base
	{
	public:
		int virtualDerived;
		VirtualDerived() : virtualDerived(4) {}
	};

	class CountingDerived : public virtual Base
	{
	public:
		int countingDerived;
		CountingDerived() : countingDerived(5)
		{
			count++;
		}
		~CountingDerived()
		{
			count--;
		}

		static int count;
	};

	int CountingDerived::count = 0;

	class Throwing
	{
	public:
		explicit Throwing(bool throwing)
		{
			if (throwing)
			{
				CAGE_THROW_ERROR(Exception, "throwing class");
			}
		}

		char data[64] = {};
	};
}

void testClasses()
{
	CAGE_TESTCASE("classes");

	{
		CAGE_TESTCASE("class_cast");
		Holder<Base> ah = detail::systemArena().createHolder<Base>();
		Holder<Base> bh = detail::systemArena().createImpl<Base, Derived>();
		Base *a = ah.get();
		Base *b = bh.get();
		CAGE_TEST(class_cast<Base*>(a));
		CAGE_TEST(class_cast<Base*>(b));
		CAGE_TEST(class_cast<Derived*>(b));
		CAGE_TEST_ASSERTED(class_cast<OtherDerived*>(b));
		Base *n = nullptr;
		CAGE_TEST(class_cast<Base*>(n) == nullptr);
		CAGE_TEST(class_cast<Derived*>(n) == nullptr);
	}

	{
		CAGE_TESTCASE("holder cast");
		CAGE_TEST((detail::systemArena().createHolder<Base>().dynCast<Base>()));
		CAGE_TEST((detail::systemArena().createImpl<Base, Derived>().dynCast<Base>()));
		CAGE_TEST((detail::systemArena().createImpl<Base, Derived>().dynCast<Derived>()));
		CAGE_TEST_THROWN((detail::systemArena().createImpl<Base, Derived>().dynCast<OtherDerived>()));
		CAGE_TEST((Holder<Base>().dynCast<Derived>().get()) == nullptr);
		CAGE_TEST((Holder<Derived>().dynCast<Derived>().get()) == nullptr);
		CAGE_TEST((Holder<Derived>().dynCast<Base>().get()) == nullptr);
	}

	{
		CAGE_TESTCASE("memory arena and throwing constructor");
		MemoryArena m = detail::systemArena();
		CAGE_TEST(m.createHolder<Throwing>(false));
		CAGE_TEST_THROWN(m.createHolder<Throwing>(true));
	}

	{
		CAGE_TESTCASE("holder and inheritance");
		MemoryArena m = detail::systemArena();
		{
			CAGE_TESTCASE("regular inheritance");
			Holder<Base> h = m.createImpl<Base, Derived>();
			CAGE_TEST(templates::move(h).dynCast<Derived>()->derived == 2);
		}
		{
			CAGE_TESTCASE("virtual inheritance");
			Holder<Base> h = m.createImpl<Base, VirtualDerived>();
			CAGE_TEST(templates::move(h).dynCast<VirtualDerived>()->virtualDerived == 4);
		}
		{
			CAGE_TESTCASE("virtual destructor");
			Holder<Base> h = m.createImpl<Base, CountingDerived>();
			CAGE_TEST(CountingDerived::count == 1);
			CAGE_TEST(templates::move(h).dynCast<CountingDerived>()->countingDerived == 5);
			CAGE_TEST(CountingDerived::count == 0);
		}
	}
}
