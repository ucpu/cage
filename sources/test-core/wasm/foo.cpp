#include <map>

struct Foo
{
	int data = 1;
	Foo() { data = 2; }
	~Foo() { data = 3; }
};

Foo foo;

extern "C"
{
	int get_foo(void)
	{
		return foo.data;
	}

	void set_foo(int v)
	{
		foo.data = v;
	}
} // extern C

std::map<int, Foo> map;

extern "C"
{
	int get_map(int key)
	{
		return map[key].data;
	}

	void set_map(int key, int val)
	{
		map[key].data = val;
	}
} // extern C
