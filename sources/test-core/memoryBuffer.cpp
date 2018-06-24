#include "main.h"
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/bufferStream.h>
#include <cage-core/utility/serialization.h>

void testMemoryBuffers()
{
	CAGE_TESTCASE("memory buffers");

	{
		CAGE_TESTCASE("compression");
		{
			memoryBuffer b1(1000);
			for (uintPtr i = 0, e = b1.size(); i < e; i++)
				((uint8*)b1.data())[i] = (uint8)i;
			memoryBuffer b2 = detail::compress(b1);
			CAGE_TEST(b2.size() < b1.size());
			memoryBuffer b3 = detail::decompress(b2, 2000);
			CAGE_TEST(b3.size() == b1.size());
			CAGE_TEST(detail::memcmp(b3.data(), b1.data(), b1.size()) == 0);
		}
		{
			memoryBuffer b1(128);
			for (uintPtr i = 0, e = b1.size(); i < e; i++)
				((uint8*)b1.data())[i] = (uint8)i;
			memoryBuffer b2 = detail::compress(b1);
			memoryBuffer b3 = detail::decompress(b2, 200);
			CAGE_TEST(b3.size() == b1.size());
			CAGE_TEST(detail::memcmp(b3.data(), b1.data(), b1.size()) == 0);
		}
	}

	{
		CAGE_TESTCASE("buffer streams");
		{
			CAGE_TESTCASE("input stream reading uint8 array");
			memoryBuffer b1(2000);
			for (uintPtr i = 0, e = b1.size(); i < e; i++)
				((uint8*)b1.data())[i] = (uint8)i;
			bufferIStream is(b1);
			for (uintPtr i = 0, e = b1.size(); i < e; i++)
			{
				uint8 c;
				is.read((char*)&c, 1);
				CAGE_TEST(c == (uint8)i);
			}
		}
		{
			CAGE_TESTCASE("input stream reading several types");
			struct dataStruct
			{
				uint8 arr[128];
				uint64 u64;
				uint16 u16;
			} data;
			detail::memset(&data, 0, sizeof(dataStruct));
			for (uint32 i = 0; i < 128; i++)
				data.arr[i] = (uint8)i;
			data.u64 = 42424242424242;
			data.u16 = 42;
			memoryBuffer b(sizeof(dataStruct));
			detail::memcpy(b.data(), &data, b.size());
			bufferIStream is(b);
			dataStruct s;
			detail::memset(&s, 0, sizeof(dataStruct));
			for (uint32 i = 0; i < 128; i++)
				is.read((char*)&s.arr[i], 1);
			CAGE_TEST(is.position() == 128);
			is.read((char*)&s.u64, 8);
			is.read((char*)&s.u16, 2);
			CAGE_TEST(detail::memcmp(&data, &s, sizeof(dataStruct)) == 0);
		}
		{
			CAGE_TESTCASE("output stream writing several types");
			struct dataStruct
			{
				uint8 arr[128];
				uint64 u64;
				uint16 u16;
			} data;
			detail::memset(&data, 0, sizeof(dataStruct));
			for (uint32 i = 0; i < 128; i++)
				data.arr[i] = (uint8)i;
			data.u64 = 42424242424242;
			data.u16 = 42;
			memoryBuffer b;
			bufferOStream os(b);
			for (uint32 i = 0; i < 128; i++)
				os.write((char*)&data.arr[i], 1);
			os.write((char*)&data.u64, 8);
			os.write((char*)&data.u16, 2);
			CAGE_TEST(b.size() >= 128 + 8 + 2 && b.size() <= sizeof(dataStruct), b.size(), sizeof(dataStruct));
			CAGE_TEST(detail::memcmp(&data, b.data(), b.size()) == 0);
		}
	}

	{
		CAGE_TESTCASE("serialization");
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
}
