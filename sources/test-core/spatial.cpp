#include <set>
#include <vector>
#include <algorithm>

#include "main.h"

#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/timer.h>
#include <cage-core/spatial.h>

namespace
{
#ifdef CAGE_DEBUG
	static const uint32 limit = 2000;
#else
	static const uint32 limit = 20000;
#endif

	const vec3 generateRandomPoint()
	{
		return randomRange3(-120, 120);
	}

	const aabb generateRandomBox()
	{
		return aabb(generateRandomPoint(), generateRandomPoint());
	}

	const aabb generateNonuniformBox()
	{
		real x = randomRange(-120, 120);
		real z = randomRange(-120, 120);
		real y = 4 * sin(rads(x * sqrt(abs(z + 2) + 0.3))) + 2 * real::E.pow(1 + cos(rads(x / 20 + (z - 40) / 30)));
		vec3 o = vec3(x, y, z);
		vec3 s = generateRandomPoint() * 0.05;
		return aabb(o + s, o - s);
	}

	struct checker
	{
		spatialDataClass *const data;
		holder<spatialQueryClass> query;

		checker(spatialDataClass *data) : data(data)
		{
			query = newSpatialQuery(data);
		}

		void checkResults(const std::set<uint32> &b)
		{
			const uint32 *res = query->resultData();
			const uint32 cnt = query->resultCount();
			CAGE_TEST(b.size() == cnt);
			std::set<uint32> a(res, res + cnt);
			for (auto ita = a.begin(), itb = b.begin(), eta = a.end(); ita != eta; ita++, itb++)
				CAGE_TEST(*ita == *itb);
		}

		void verifiableAabb(const aabb elData[], const uint32 elCount)
		{
			aabb qb = generateRandomBox();
			query->intersection(qb);
			std::set<uint32> b;
			for (uint32 i = 0; i < elCount; i++)
			{
				if (intersects(elData[i], qb))
					b.insert(i);
			}
			checkResults(b);
		}

		void verifiableRange(const aabb elData[], const uint32 elCount)
		{
			sphere sph(generateRandomPoint(), cage::randomRange(10, 100));
			query->intersection(sph);
			std::set<uint32> b;
			for (uint32 i = 0; i < elCount; i++)
			{
				if (intersects(elData[i], sph))
					b.insert(i);
			}
			checkResults(b);
		}

		void randomAabb()
		{
			query->intersection(generateRandomBox());
		}

		void randomRange()
		{
			query->intersection(sphere(generateRandomPoint(), cage::randomRange(10, 100)));
		}
	};

	void verifiableQueries(const aabb elData[], const uint32 elCount, spatialDataClass *data)
	{
		checker c(data);
		for (uint32 qi = 0; qi < 30; qi++)
			c.verifiableAabb(elData, elCount);
		for (uint32 qi = 0; qi < 30; qi++)
			c.verifiableRange(elData, elCount);
	}

	void randomQueries(spatialDataClass *data)
	{
		checker c(data);
		for (uint32 qi = 0; qi < 30; qi++)
			c.randomAabb();
		for (uint32 qi = 0; qi < 30; qi++)
			c.randomRange();
	}
}

void testSpatial()
{
	CAGE_TESTCASE("spatial");

	{
		CAGE_TESTCASE("basic tests");
		holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
		std::vector<aabb> elements;

		// insertions
		elements.push_back(aabb());
		for (uint32 k = 1; k < limit / 2; k++)
		{
			aabb b = generateRandomBox();
			elements.push_back(b);
			data->update(k, b);
		}
		data->rebuild();
		verifiableQueries(&elements[0], numeric_cast<uint32>(elements.size()), data.get());

		// updates
		for (uint32 i = 0; i < limit / 5; i++)
		{
			uint32 k = randomRange((uint32)1, numeric_cast<uint32>(elements.size()));
			aabb b = generateRandomBox();
			elements[k] = b;
			data->update(k, b);
		}
		data->rebuild();
		verifiableQueries(&elements[0], numeric_cast<uint32>(elements.size()), data.get());

		// removes
		for (uint32 i = 0; i < limit / 5; i++)
		{
			uint32 k = randomRange((uint32)1, numeric_cast<uint32>(elements.size()));
			elements[k] = aabb();
			data->remove(k);
		}
		data->rebuild();
		verifiableQueries(&elements[0], numeric_cast<uint32>(elements.size()), data.get());
	}

	{
		CAGE_TESTCASE("points");
		holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
		for (uint32 i = 1; i < 100; i++)
			data->update(i, aabb(generateRandomPoint()));
		data->rebuild();
		holder<spatialQueryClass> query = newSpatialQuery(data.get());
		query->intersection(sphere(vec3(50, 0, 0), 100));
	}

	{
		CAGE_TESTCASE("multiple points on same location");
		static const vec3 pts[3] = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) };
		holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
		for (uint32 i = 1; i < 100; i++)
			data->update(i, aabb(pts[i % 3]));
		data->rebuild();
		holder<spatialQueryClass> query = newSpatialQuery(data.get());
		query->intersection(sphere(vec3(0, 0, 0), 5));
		CAGE_TEST(query->resultCount() == 99);
	}

	{
		CAGE_TESTCASE("spheres");
		holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
		data->update(1, sphere(vec3(), 100));
		data->rebuild();
		holder<spatialQueryClass> query = newSpatialQuery(data.get());
		query->intersection(sphere(vec3(50, 0, 0), 100));
		CAGE_TEST(query->resultCount() == 1);

		// test aabb-sphere intersection
		query->intersection(generateRandomBox());
	}

	{
		CAGE_TESTCASE("performance tests");
		holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
		holder<timerClass> tmr = newTimer();

		for (uint32 i = 0; i < limit; i++)
		{
			uint32 k = randomRange((uint32)1, limit / 100);
			if (i > limit / 3 && randomChance() < 0.3)
				data->remove(k);
			else
				data->update(k, generateNonuniformBox());
			if (i % 20 == 0)
			{
				data->rebuild();
				randomQueries(data.get());
			}
		}

		CAGE_LOG(severityEnum::Info, "spatial performance", string() + "total time: " + tmr->microsSinceStart() + " us");
	}
}
