#include "main.h"
#include <cage-core/memory.h>

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
		Throwing(bool throwing)
		{
			if (throwing)
			{
				CAGE_THROW_ERROR(exception, "throwing class");
			}
		}

		char data[64];
	};

	typedef memoryArenaFixed<memoryAllocatorPolicyPool<128, memoryBoundsPolicySimple, memoryTagPolicySimple, memoryTrackPolicyAdvanced>, memoryConcurrentPolicyNone> Arena;
}

void testClasses()
{
	CAGE_TESTCASE("classes");

	{
		CAGE_TESTCASE("class_cast");
		Derived derived;
		Base *base = &derived;
		CAGE_TEST(class_cast<Derived*>(base) != nullptr);
		CAGE_TEST_ASSERTED(class_cast<OtherDerived*>(base));
	}

	{
		CAGE_TESTCASE("memory arena and throwing constructor");
		Arena arena(1024 * 1024);
		memoryArena m(&arena);
		CAGE_TEST(m.createHolder<Throwing>(false));
		CAGE_TEST_THROWN(m.createHolder<Throwing>(true));
	}

	{
		CAGE_TESTCASE("holder and inheritance");
		Arena arena(1024 * 1024);
		memoryArena m(&arena);
		{
			CAGE_TESTCASE("regular inheritance");
			holder<Base> h = m.createImpl<Base, Derived>();
			CAGE_TEST(h.cast<Derived>()->derived == 2);
		}
		{
			CAGE_TESTCASE("virtual inheritance");
			holder<Base> h = m.createImpl<Base, VirtualDerived>();
			CAGE_TEST(h.cast<VirtualDerived>()->virtualDerived == 4);
		}
		{
			CAGE_TESTCASE("virtual destructor");
			holder<Base> h = m.createImpl<Base, CountingDerived>();
			CAGE_TEST(CountingDerived::count == 1);
			CAGE_TEST(h.cast<CountingDerived>()->countingDerived == 5);
			h.clear();
			CAGE_TEST(CountingDerived::count == 0);
		}
	}
}
