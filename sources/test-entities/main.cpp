#include <algorithm>
#include <array>
#include <vector>

#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/logger.h>
#include <cage-core/random.h>
#include <cage-core/timer.h>

using namespace cage;

volatile uint32 globalVolatile = 0; // avoid eliminating code that is used to simulate memory access or computations

template<uint32 Count>
struct Value
{
	uint32 data[Count];

	void update()
	{
		for (uint32 &i : data)
			i++;
	}

	void read()
	{
		uint32 sum = 0;
		for (uint32 i : data)
			sum += i;
		globalVolatile += sum;
	}
};

using V0 = Value<1>;
using V1 = Value<5>;
using V2 = Value<2>;
using V3 = Value<4>;
using V4 = Value<10>;
using V5 = Value<6>;
using V6 = Value<12>;
using V7 = Value<20>;
using V8 = Value<16>;
using V9 = Value<3>;

struct Performance
{
	Holder<EntityManager> man = newEntityManager();
	RandomGenerator rng = RandomGenerator(123457, 789159753);

	Performance()
	{
		man->defineComponent(V0());
		man->defineComponent(V1());
		man->defineComponent(V2());
		man->defineComponent(V3());
		man->defineComponent(V4());
		man->defineComponent(V5());
		man->defineComponent(V6());
		man->defineComponent(V7());
		man->defineComponent(V8());
		man->defineComponent(V9());
	}

	void add()
	{
		const uint32 cnt = 10; // rng.randomRange(3, 10);
		for (uint32 i = 0; i < cnt; i++)
		{
			switch (rng.next() % 20)
			{
				case 0:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V3>();
					break;
				}
				case 1:
				{
					Entity *e = man->createUnique();
					e->value<V1>();
					e->value<V5>();
					break;
				}
				case 2:
				{
					Entity *e = man->createUnique();
					e->value<V2>();
					e->value<V4>();
					break;
				}
				case 3:
				{
					Entity *e = man->createUnique();
					e->value<V7>();
					e->value<V8>();
					e->value<V9>();
					break;
				}
				case 4:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V2>();
					e->value<V3>();
					break;
				}
				case 5:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V7>();
					e->value<V8>();
					break;
				}
				case 6:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V2>();
					e->value<V5>();
					break;
				}
				case 7:
				{
					Entity *e = man->createUnique();
					e->value<V1>();
					e->value<V3>();
					e->value<V9>();
					break;
				}
				case 8:
				{
					Entity *e = man->createUnique();
					e->value<V2>();
					e->value<V3>();
					e->value<V6>();
					break;
				}
				case 9:
				{
					Entity *e = man->createUnique();
					e->value<V3>();
					e->value<V4>();
					e->value<V9>();
					break;
				}
				case 10:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V4>();
					e->value<V6>();
					break;
				}
				case 11:
				{
					Entity *e = man->createUnique();
					e->value<V6>();
					e->value<V8>();
					e->value<V9>();
					break;
				}
				case 12:
				{
					Entity *e = man->createUnique();
					e->value<V4>();
					e->value<V6>();
					e->value<V7>();
					break;
				}
				case 13:
				{
					Entity *e = man->createUnique();
					e->value<V3>();
					e->value<V5>();
					e->value<V7>();
					break;
				}
				case 14:
				{
					Entity *e = man->createUnique();
					e->value<V4>();
					e->value<V6>();
					e->value<V7>();
					break;
				}
				case 15:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V3>();
					e->value<V4>();
					e->value<V5>();
					e->value<V7>();
					e->value<V8>();
					break;
				}
				case 16:
				{
					Entity *e = man->createUnique();
					e->value<V1>();
					e->value<V2>();
					e->value<V4>();
					e->value<V6>();
					e->value<V7>();
					e->value<V9>();
					break;
				}
				case 17:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V3>();
					e->value<V4>();
					e->value<V5>();
					e->value<V6>();
					e->value<V9>();
					break;
				}
				case 18:
				{
					Entity *e = man->createUnique();
					e->value<V1>();
					e->value<V2>();
					e->value<V5>();
					e->value<V6>();
					e->value<V7>();
					e->value<V8>();
					break;
				}
				case 19:
				{
					Entity *e = man->createUnique();
					e->value<V0>();
					e->value<V1>();
					e->value<V3>();
					e->value<V4>();
					e->value<V6>();
					e->value<V8>();
					break;
				}
			}
		}
	}

	Entity *pick()
	{
		const auto arr = man->entities();
		return arr[rng.randomRange(uintPtr(), arr.size())];
	}

	void remove()
	{
		const uint32 cnt = 10; // rng.randomRange(3, 10);
		for (uint32 i = 0; i < cnt; i++)
		{
			// remove youngest entity from a random selection
			std::array<Entity *, 4> arr = { pick(), pick(), pick(), pick() };
			std::sort(arr.begin(), arr.end(), [](Entity *a, Entity *b) { return a->name() > b->name(); });
			arr[0]->destroy();
		}
	}

	void modify()
	{
		const uint32 cnt = rng.randomRange(10, 20);
		for (uint32 i = 0; i < cnt; i++)
		{
			Entity *e = pick();
			switch (rng.next() % 10)
			{
				case 0:
				{
					e->remove<V0>();
					e->value<V1>();
					break;
				}
				case 1:
				{
					e->remove<V1>();
					e->value<V2>();
					break;
				}
				case 2:
				{
					e->remove<V2>();
					e->value<V3>();
					break;
				}
				case 3:
				{
					e->remove<V3>();
					e->value<V4>();
					break;
				}
				case 4:
				{
					e->remove<V4>();
					e->value<V5>();
					break;
				}
				case 5:
				{
					e->remove<V5>();
					e->value<V6>();
					break;
				}
				case 6:
				{
					e->remove<V6>();
					e->value<V7>();
					break;
				}
				case 7:
				{
					e->remove<V7>();
					e->value<V8>();
					break;
				}
				case 8:
				{
					e->remove<V8>();
					e->value<V9>();
					break;
				}
				case 9:
				{
					e->remove<V9>();
					e->value<V0>();
					break;
				}
			}
		}
	}

	void access()
	{
		switch (rng.next() % 10)
		{
			case 0:
			{
				entitiesVisitor(
					[](Entity *, V1 &a, V2 &b, V3 &c)
					{
						a.read();
						b.read();
						c.update();
					},
					+man, false);
				break;
			}
			case 1:
			{
				entitiesVisitor(
					[](Entity *, V4 &a, V5 &b, V6 &c)
					{
						a.update();
						b.read();
						c.read();
					},
					+man, false);
				break;
			}
			case 2:
			{
				entitiesVisitor(
					[](Entity *, V7 &a, V8 &b, V9 &c)
					{
						a.update();
						b.update();
						c.read();
					},
					+man, false);
				break;
			}
			case 3:
			{
				entitiesVisitor(
					[](Entity *, V0 &a, V3 &b, V4 &c)
					{
						a.read();
						b.read();
						c.read();
					},
					+man, false);
				break;
			}
			case 4:
			{
				entitiesVisitor(
					[](Entity *, V2 &a, V7 &b)
					{
						a.read();
						b.read();
					},
					+man, false);
				break;
			}
			case 5:
			{
				entitiesVisitor(
					[](Entity *, V3 &a, V5 &b, V7 &c)
					{
						a.read();
						b.update();
						c.read();
					},
					+man, false);
				break;
			}
			case 6:
			{
				entitiesVisitor(
					[](Entity *, V0 &a, V6 &b, V9 &c)
					{
						a.read();
						b.update();
						c.read();
					},
					+man, false);
				break;
			}
			case 7:
			{
				entitiesVisitor(
					[](Entity *, V0 &a, V2 &b, V6 &c)
					{
						a.update();
						b.read();
						c.read();
					},
					+man, false);
				break;
			}
			case 8:
			{
				entitiesVisitor(
					[](Entity *, V1 &a, V5 &b, V7 &c)
					{
						a.read();
						b.read();
						c.update();
					},
					+man, false);
				break;
			}
			case 9:
			{
				entitiesVisitor(
					[](Entity *, V0 &a, V2 &b, V3 &c, V7 &d, V9 &e)
					{
						a.read();
						b.read();
						c.read();
						d.read();
						e.update();
					},
					+man, false);
				break;
			}
		}
	}

	void run()
	{
		static constexpr uint32 Initialization = CAGE_DEBUG_BOOL ? 50 : 200;
		static constexpr uint32 Iterations = CAGE_DEBUG_BOOL ? 10000 : 100000;
		for (uint32 i = 0; i < Initialization; i++)
			add();
		for (uint32 i = 0; i < Iterations; i++)
		{
			add();
			access();
			modify();
			access();
			remove();
		}
	}
};

void performanceLoop()
{
	std::vector<uint64> timings;
	Holder<Timer> timer = newTimer();
	for (uint32 i = 0; i < 10; i++)
	{
		Performance p;
		timer->reset();
		p.run();
		const uint64 d = timer->duration();
		timings.push_back(d);
		CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "duration: " + d + " us");
	}
	std::sort(timings.begin(), timings.end());
	timings.pop_back(); // ignore slowest
	timings.pop_back();
	timings.erase(timings.begin()); // ignore fastest
	timings.erase(timings.begin());
	uint64 sum = 0;
	for (uint64 i : timings)
		sum += i;
	sum /= timings.size();
	CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "average: " + sum + " us");
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		performanceLoop();
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
