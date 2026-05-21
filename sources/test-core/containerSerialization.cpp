#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <cage-core/containerSerialization.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/stdHash.h>
#include <cage-core/variantSerialization.h>

#include "main.h"

void testContainerSerialization()
{
	CAGE_TESTCASE("container serialization");

	{
		static_assert(!privat::WritableContainerConcept<uint32>);
		static_assert(!privat::WritableContainerConcept<String>);
		static_assert(privat::WritableContainerConcept<std::vector<uint32>>);
		static_assert(privat::WritableContainerConcept<std::vector<String>>);
		static_assert(privat::WritableContainerConcept<std::set<uint32>>);
		static_assert(privat::WritableContainerConcept<std::set<String>>);
		static_assert(privat::WritableContainerConcept<std::map<String, uint32>>);
		static_assert(privat::WritableContainerConcept<std::map<uint32, String>>);
		static_assert(privat::WritableContainerConcept<std::vector<std::vector<uint32>>>);

		static_assert(privat::MemcpyContainerConcept<std::vector<char>>);
		static_assert(privat::MemcpyContainerConcept<std::vector<uint32>>);
		static_assert(!privat::MemcpyContainerConcept<std::vector<String>>);
		static_assert(privat::MemcpyContainerConcept<std::vector<Real>>);
		static_assert(privat::MemcpyContainerConcept<std::vector<Vec3>>);
	}

	{
		CAGE_TESTCASE("vector of strings");
		std::vector<String> cont;
		cont.push_back("hello");
		cont.push_back("darling");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0] == "hello");
		CAGE_TEST(cont[1] == "darling");
		CAGE_TEST(buf.size() == 8 + 4 + 5 + 4 + 7); // make sure that the behavior is consistent for both 32 and 64 bit applications
	}

	{
		CAGE_TESTCASE("vector of uint32");
		std::vector<uint32> cont;
		cont.push_back(13);
		cont.push_back(42);
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0] == 13);
		CAGE_TEST(cont[1] == 42);
		CAGE_TEST(buf.size() == 8 + 4 + 4); // make sure that the behavior is consistent for both 32 and 64 bit applications
	}

	{
		CAGE_TESTCASE("list");
		std::list<String> cont;
		cont.push_back("hello");
		cont.push_back("darling");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont.front() == "hello");
		CAGE_TEST(cont.back() == "darling");
	}

	{
		CAGE_TESTCASE("map");
		std::map<String, String> cont;
		cont["abc"] = "def";
		cont["ghi"] = "jkl";
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont["abc"] == "def");
		CAGE_TEST(cont["ghi"] == "jkl");
	}

	{
		CAGE_TESTCASE("unordered_map");
		std::unordered_map<String, String> cont;
		cont["abc"] = "def";
		cont["ghi"] = "jkl";
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont["abc"] == "def");
		CAGE_TEST(cont["ghi"] == "jkl");
	}

	{
		CAGE_TESTCASE("set");
		std::set<String> cont;
		cont.insert("hello");
		cont.insert("darling");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont.count("hello") == 1);
		CAGE_TEST(cont.count("darling") == 1);
	}

	{
		CAGE_TESTCASE("unordered_set");
		std::unordered_set<String> cont;
		cont.insert("hello");
		cont.insert("darling");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont.count("hello") == 1);
		CAGE_TEST(cont.count("darling") == 1);
	}

	{
		CAGE_TESTCASE("vector of vectors");
		std::vector<std::vector<String>> cont;
		cont.resize(2);
		cont[0].push_back("a");
		cont[0].push_back("b");
		cont[0].push_back("c");
		cont[1].push_back("d");
		cont[1].push_back("e");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0].size() == 3);
		CAGE_TEST(cont[1].size() == 2);
		CAGE_TEST(cont[0][0] == "a");
		CAGE_TEST(cont[0][1] == "b");
		CAGE_TEST(cont[0][2] == "c");
		CAGE_TEST(cont[1][0] == "d");
		CAGE_TEST(cont[1][1] == "e");
	}

	{
		CAGE_TESTCASE("map of vectors");
		std::map<String, std::vector<String>> cont;
		cont["1"].push_back("a");
		cont["1"].push_back("b");
		cont["1"].push_back("c");
		cont["2"].push_back("d");
		cont["2"].push_back("e");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont["1"].size() == 3);
		CAGE_TEST(cont["2"].size() == 2);
		CAGE_TEST(cont["1"][0] == "a");
		CAGE_TEST(cont["1"][1] == "b");
		CAGE_TEST(cont["1"][2] == "c");
		CAGE_TEST(cont["2"][0] == "d");
		CAGE_TEST(cont["2"][1] == "e");
	}

	{
		CAGE_TESTCASE("r-value deserializer for vector");
		std::vector<String> cont;
		cont.push_back("hello");
		cont.push_back("darling");
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des.copy() >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0] == "hello");
		CAGE_TEST(cont[1] == "darling");
	}

	{
		CAGE_TESTCASE("pointer range (string)");
		std::vector<String> cont;
		cont.push_back("hello");
		cont.push_back("darling");
		PointerRange<const String> range = cont;
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << range;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0] == "hello");
		CAGE_TEST(cont[1] == "darling");
		CAGE_TEST(buf.size() == 8 + 4 + 5 + 4 + 7);
	}

	{
		CAGE_TESTCASE("pointer range (uint32)");
		std::vector<uint32> cont;
		cont.push_back(13);
		cont.push_back(42);
		PointerRange<const uint32> range = cont;
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << range;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 2);
		CAGE_TEST(cont[0] == 13);
		CAGE_TEST(cont[1] == 42);
		CAGE_TEST(buf.size() == 8 + 4 + 4);
	}

	{
		CAGE_TESTCASE("flat set");
		FlatSet<uint32> cont;
		cont.insert(42);
		cont.insert(13);
		cont.insert(20);
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 3);
		CAGE_TEST(cont.data()[0] == 13);
		CAGE_TEST(cont.data()[1] == 20);
		CAGE_TEST(cont.data()[2] == 42);
	}

	{
		CAGE_TESTCASE("vector of bool");
		std::vector<bool> cont;
		cont.push_back(true);
		cont.push_back(false);
		cont.push_back(true);
		cont.push_back(true);
		cont.push_back(true);
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		cont.clear();
		Deserializer des(buf);
		des >> cont;
		CAGE_TEST(cont.size() == 5);
		CAGE_TEST(cont[0] == true);
		CAGE_TEST(cont[1] == false);
		CAGE_TEST(cont[2] == true);
		CAGE_TEST(cont[3] == true);
		CAGE_TEST(cont[4] == true);
		CAGE_TEST(buf.size() == 8 + 1); // make sure that the three bits take one byte only
	}

	{
		CAGE_TESTCASE("randomized vector of bool");
		std::vector<bool> cont;
		const uint32 size = rand() % 1000;
		for (uint32 i = 0; i < size; i++)
			cont.push_back((rand() % 2) == 0);
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << cont;
		std::vector<bool> cont2;
		Deserializer des(buf);
		des >> cont2;
		CAGE_TEST(cont == cont2);
		CAGE_TEST(buf.size() < size / 8 + 10);
	}

	{
		CAGE_TESTCASE("variant");
		using Var = std::variant<std::monostate, String, uint32, Vec3>;
		Var a = "hello there";
		Var b = (uint32)42;
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << a << b;
		Var c, d;
		Deserializer des(buf);
		des >> c >> d;
		CAGE_TEST(std::holds_alternative<String>(c));
		CAGE_TEST(std::holds_alternative<uint32>(d));
		CAGE_TEST(std::get<String>(c) == "hello there");
		CAGE_TEST(std::get<uint32>(d) == 42);
		CAGE_TEST(buf.size() < sizeof(String));
	}
}
