#include "main.h"

#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/timer.h>
#include <cage-core/spatialStructure.h>

#include <set>
#include <vector>
#include <algorithm>

namespace
{
#ifdef CAGE_DEBUG
	constexpr uint32 limit = 2000;
#else
	constexpr uint32 limit = 20000;
#endif

	vec3 generateRandomPoint()
	{
		return randomRange3(-120, 120);
	}

	Aabb generateRandomBox()
	{
		return Aabb(generateRandomPoint(), generateRandomPoint());
	}

	Aabb generateNonuniformBox()
	{
		real x = randomRange(-120, 120);
		real z = randomRange(-120, 120);
		real y = 4 * sin(rads(x * sqrt(abs(z + 2) + 0.3))) + 2 * powE(1 + cos(rads(x / 20 + (z - 40) / 30)));
		vec3 o = vec3(x, y, z);
		vec3 s = generateRandomPoint() * 0.05;
		return Aabb(o + s, o - s);
	}

	struct Checker
	{
		const Holder<const SpatialStructure> data;
		Holder<SpatialQuery> query;

		Checker(Holder<const SpatialStructure> data) : data(data.share())
		{
			query = newSpatialQuery(data.share());
		}

		void checkResults(const std::set<uint32> &b)
		{
			CAGE_TEST(query->result().size() == b.size());
			std::set<uint32> a(query->result().begin(), query->result().end());
			for (auto ita = a.begin(), itb = b.begin(), eta = a.end(); ita != eta; ita++, itb++)
				CAGE_TEST(*ita == *itb);
		}

		void verifiableAabb(const Aabb elData[], const uint32 elCount)
		{
			Aabb qb = generateRandomBox();
			query->intersection(qb);
			std::set<uint32> b;
			for (uint32 i = 0; i < elCount; i++)
			{
				if (intersects(elData[i], qb))
					b.insert(i);
			}
			checkResults(b);
		}

		void verifiableRange(const Aabb elData[], const uint32 elCount)
		{
			Sphere sph(generateRandomPoint(), cage::randomRange(10, 100));
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
			query->intersection(Sphere(generateRandomPoint(), cage::randomRange(10, 100)));
		}
	};

	void verifiableQueries(const Aabb elData[], const uint32 elCount, Holder<const SpatialStructure> data)
	{
		Checker c(std::move(data));
		for (uint32 qi = 0; qi < 30; qi++)
			c.verifiableAabb(elData, elCount);
		for (uint32 qi = 0; qi < 30; qi++)
			c.verifiableRange(elData, elCount);
	}

	void randomQueries(Holder<const SpatialStructure> data)
	{
		Checker c(std::move(data));
		for (uint32 qi = 0; qi < 30; qi++)
			c.randomAabb();
		for (uint32 qi = 0; qi < 30; qi++)
			c.randomRange();
	}
}

void testSpatialStructure()
{
	CAGE_TESTCASE("spatial");

	{
		CAGE_TESTCASE("basic tests");
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		std::vector<Aabb> elements;

		// insertions
		for (uint32 k = 0; k < limit / 2; k++)
		{
			Aabb b = generateRandomBox();
			elements.push_back(b);
			data->update(k, b);
		}
		data->rebuild();
		verifiableQueries(elements.data(), numeric_cast<uint32>(elements.size()), data.share());

		// updates
		for (uint32 i = 0; i < limit / 5; i++)
		{
			uint32 k = randomRange(0u, numeric_cast<uint32>(elements.size()));
			Aabb b = generateRandomBox();
			elements[k] = b;
			data->update(k, b);
		}
		data->rebuild();
		verifiableQueries(elements.data(), numeric_cast<uint32>(elements.size()), data.share());

		// removes
		for (uint32 i = 0; i < limit / 5; i++)
		{
			uint32 k = randomRange(0u, numeric_cast<uint32>(elements.size()));
			elements[k] = Aabb();
			data->remove(k);
		}
		data->rebuild();
		verifiableQueries(elements.data(), numeric_cast<uint32>(elements.size()), data.share());
	}

	{
		CAGE_TESTCASE("points");
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		for (uint32 i = 0; i < 100; i++)
			data->update(i, Aabb(generateRandomPoint()));
		data->rebuild();
		Holder<SpatialQuery> query = newSpatialQuery(data.share());
		query->intersection(Sphere(vec3(50, 0, 0), 100));
	}

	{
		CAGE_TESTCASE("multiple points on same location");
		constexpr const vec3 pts[3] = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) };
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		for (uint32 i = 0; i < 100; i++)
			data->update(i, Aabb(pts[i % 3]));
		data->rebuild();
		Holder<SpatialQuery> query = newSpatialQuery(data.share());
		query->intersection(Sphere(vec3(0, 0, 0), 5));
		CAGE_TEST(query->result().size() == 100);
	}

	{
		CAGE_TESTCASE("spheres");
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		data->update(0, Sphere(vec3(), 100));
		data->rebuild();
		Holder<SpatialQuery> query = newSpatialQuery(data.share());
		query->intersection(Sphere(vec3(50, 0, 0), 100));
		CAGE_TEST(query->result().size() == 1);
		query->intersection(Sphere(vec3(250, 0, 0), 100));
		CAGE_TEST(query->result().size() == 0);

		// test aabb-sphere intersection
		query->intersection(generateRandomBox());
	}

	{
		CAGE_TESTCASE("multiple spheres on same position");
		constexpr const vec3 pts[3] = { vec3(3, 0, 0), vec3(0, 7, 0), vec3(0, 0, 13) };
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		for (uint32 i = 0; i < 100; i++)
			data->update(i, Sphere(pts[i % 3], 1));
		data->rebuild();
		Holder<SpatialQuery> query = newSpatialQuery(data.share());
		query->intersection(Sphere(vec3(0, 0, 0), 5));
		CAGE_TEST(query->result().size() == 34);
	}

	{
		CAGE_TESTCASE("performance tests");
		Holder<SpatialStructure> data = newSpatialStructure(SpatialStructureCreateConfig());
		Holder<Timer> tmr = newTimer();

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
				randomQueries(data.share());
			}
		}

		CAGE_LOG(SeverityEnum::Info, "spatial performance", stringizer() + "total time: " + tmr->duration() + " us");
	}
}
