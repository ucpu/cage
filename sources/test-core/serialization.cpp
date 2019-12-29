#include "main.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

void testSerialization()
{
	CAGE_TESTCASE("serialization");

	{
		CAGE_TESTCASE("basics");
		{
			MemoryBuffer b1;
			Serializer s(b1);
			s << 42 << string("hi there") << 13;
			CAGE_TEST(b1.size() == sizeof(42) + (4 + 8) + sizeof(13));
			int i42, i13;
			string hi;
			Deserializer d(b1);
			d >> i42 >> hi >> i13;
			CAGE_TEST(i42 == 42);
			CAGE_TEST(i13 == 13);
			CAGE_TEST(hi == "hi there");
		}
	}

	{
		CAGE_TESTCASE("placeholders");
		MemoryBuffer b1;
		Serializer s(b1);
		Serializer s1 = s.placeholder(8);
		CAGE_TEST(b1.size() == 1 * 8);
		Serializer s2 = s.placeholder(8);
		CAGE_TEST(b1.size() == 2 * 8);
		Serializer s3 = s.placeholder(8);
		CAGE_TEST(b1.size() == 3 * 8);
		s << 4.0;
		CAGE_TEST(b1.size() == 4 * 8);
		s1 << 1.0;
		CAGE_TEST(b1.size() == 4 * 8);
		s3 << 3.0;
		CAGE_TEST(b1.size() == 4 * 8);
		s2 << 2.0;
		CAGE_TEST(b1.size() == 4 * 8);
		double t;
		Deserializer d(b1);
		Deserializer d1 = d.placeholder(8);
		d >> t;
		CAGE_TEST(t == 2.0);
		Deserializer d3 = d.placeholder(8);
		Deserializer d4 = d.placeholder(8);
		d1 >> t;
		CAGE_TEST(t == 1.0);
		d3 >> t;
		CAGE_TEST(t == 3.0);
		d4 >> t;
		CAGE_TEST(t == 4.0);
		CAGE_TEST(b1.size() == 4 * 8);
	}

	{
		CAGE_TESTCASE("moving");
		MemoryBuffer b1;
		Serializer s1(b1);
		Deserializer d1(b1);
		MemoryBuffer b2;
		Serializer s2(b2);
		Deserializer d2(b2);
		s1 = templates::move(s2);
		d2 = templates::move(d1);
	}
}
