#include "main.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/stdBufferStream.h>

void testMemoryBuffers()
{
	CAGE_TESTCASE("memory buffers");

	{
		CAGE_TESTCASE("methods & size & capacity");
		memoryBuffer b(13, 42);
		CAGE_TEST(b.size() == 13);
		CAGE_TEST(b.capacity() == 42);
		b.allocate(20);
		CAGE_TEST(b.size() == 20);
		CAGE_TEST(b.capacity() == 20);
		b.reserve(30);
		CAGE_TEST(b.size() == 20);
		CAGE_TEST(b.capacity() == 30);
		b.resize(10);
		CAGE_TEST(b.size() == 10);
		CAGE_TEST(b.capacity() == 30);
		CAGE_TEST_THROWN(b.resizeThrow(50));
		CAGE_TEST(b.size() == 10);
		CAGE_TEST(b.capacity() == 30);
		b.resizeThrow(20);
		CAGE_TEST(b.size() == 20);
		CAGE_TEST(b.capacity() == 30);
		b.resize(50);
		CAGE_TEST(b.size() == 50);
		CAGE_TEST(b.capacity() == 50);
		b.reserve(100);
		CAGE_TEST(b.size() == 50);
		CAGE_TEST(b.capacity() == 100);
		b.shrink();
		CAGE_TEST(b.size() == 50);
		CAGE_TEST(b.capacity() == 50);
		b.clear();
		CAGE_TEST(b.size() == 0);
		CAGE_TEST(b.capacity() == 50);
		b.resizeSmart(1000);
		CAGE_TEST(b.size() == 1000);
		CAGE_TEST(b.capacity() > 1000);
		b.resizeSmart(10);
		CAGE_TEST(b.size() == 10);
		CAGE_TEST(b.capacity() > 1000);
		b.free();
		CAGE_TEST(b.size() == 0);
		CAGE_TEST(b.capacity() == 0);
	}

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
		CAGE_TESTCASE("std buffer streams");
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
}
