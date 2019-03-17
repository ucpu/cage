#include "main.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

void testSerialization()
{
	CAGE_TESTCASE("serialization");

	{
		CAGE_TESTCASE("basics");
		{
			memoryBuffer b1;
			serializer s(b1);
			s << 42 << string("hi there") << 13;
			CAGE_TEST(b1.size() == sizeof(42) + (4 + 8) + sizeof(13));
			int i42, i13;
			string hi;
			deserializer d(b1);
			d >> i42 >> hi >> i13;
			CAGE_TEST(i42 == 42);
			CAGE_TEST(i13 == 13);
			CAGE_TEST(hi == "hi there");
		}
	}

	{
		CAGE_TESTCASE("placeholders");
		memoryBuffer b1;
		serializer s(b1);
		serializer s1 = s.placeholder(8);
		CAGE_TEST(b1.size() == 1 * 8);
		serializer s2 = s.placeholder(8);
		CAGE_TEST(b1.size() == 2 * 8);
		serializer s3 = s.placeholder(8);
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
		deserializer d(b1);
		deserializer d1 = d.placeholder(8);
		d >> t;
		CAGE_TEST(t == 2.0);
		deserializer d3 = d.placeholder(8);
		deserializer d4 = d.placeholder(8);
		d1 >> t;
		CAGE_TEST(t == 1.0);
		d3 >> t;
		CAGE_TEST(t == 3.0);
		d4 >> t;
		CAGE_TEST(t == 4.0);
		CAGE_TEST(b1.size() == 4 * 8);
	}
}
