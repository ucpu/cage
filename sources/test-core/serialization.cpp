#include "main.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace
{
	struct Item
	{
		uint64 a = 42;
		uint32 b = 13;
	};

	void functionTakingSingleItem(Item &data) {}
	void functionTakingPointerRangeOfItems(PointerRange<Item> data) {}
	void functionTakingPointerRangeBuffer(PointerRange<char> buffer) {}

	void functionTakingRawArrayOfItems(Item *data, uint32 cnt) {}
	void functionTakingVoidPointer(void *data, uintPtr bytes) {}
	void functionTakingCharPointer(char *data, uintPtr bytes) {}
}

void testSerialization()
{
	CAGE_TESTCASE("serialization");

	{
		CAGE_TESTCASE("basics");
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
		CAGE_TESTCASE("lines");
		MemoryBuffer b1;
		Serializer s(b1);
		s.writeLine("hello there");
		s.writeLine("how are you?");
		Deserializer d(b1);
		string l;
		CAGE_TEST(d.readLine(l));
		CAGE_TEST(l == "hello there");
		CAGE_TEST(d.readLine(l));
		CAGE_TEST(l == "how are you?");
		CAGE_TEST(!d.readLine(l));
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

	{
		CAGE_TESTCASE("buffer casts");

		{
			CAGE_TESTCASE("from single item");
			Item item;

			functionTakingSingleItem(item);
			functionTakingPointerRangeOfItems({ &item, &item + 1 });
			functionTakingPointerRangeBuffer(bufferCast<char, Item>({ &item, &item + 1 }));

			functionTakingRawArrayOfItems(&item, 1);
			functionTakingVoidPointer(&item, sizeof(Item));
			functionTakingCharPointer((char *)&item, sizeof(Item));
		}

		{
			CAGE_TESTCASE("from items array");
			Item itemsArray[10];
			PointerRange<Item> items = itemsArray;

			functionTakingSingleItem(items[0]);
			functionTakingPointerRangeOfItems(items);
			functionTakingPointerRangeBuffer(bufferCast<char>(items));

			functionTakingRawArrayOfItems(items.data(), numeric_cast<uint32>(items.size()));
			functionTakingVoidPointer(items.data(), items.size() * sizeof(Item));
			functionTakingCharPointer((char *)items.data(), items.size() * sizeof(Item));
		}

		{
			CAGE_TESTCASE("from char buffer");
			Item itemsArray[10];
			PointerRange<char> items = bufferCast<char, Item>(itemsArray);

			functionTakingSingleItem(bufferCast<Item>(items)[0]);
			functionTakingPointerRangeOfItems(bufferCast<Item>(items));
			functionTakingPointerRangeBuffer(items);

			functionTakingRawArrayOfItems((Item*)items.data(), numeric_cast<uint32>(items.size()));
			functionTakingVoidPointer(items.data(), items.size());
			functionTakingCharPointer(items.data(), items.size());
		}
	}
}
