#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/utility/commandBucket.h>

namespace cage
{
	commandBucket::cmdNode::cmdNode() : next(nullptr), inst(nullptr), fnc(nullptr)
	{}

	void commandBucket::addNode()
	{
		cmdNode *n = arena.createObject <cmdNode>();
		if (last)
		{
			last->next = n;
			last = n;
		}
		else
		{
			first = last = n;
		}
	}

	commandBucket::commandBucket(memoryArena &arena) : arena(arena), first(nullptr), last(nullptr)
	{}

	commandBucket::~commandBucket()
	{
		flush();
	}

	void commandBucket::dispatch()
	{
		cmdNode *ptr = first;
		while (ptr)
		{
			ptr->invoke();
			ptr = ptr->next;
		}
	}

	void commandBucket::flush()
	{
		first = nullptr;
		last = nullptr;
		arena.flush();
	}
}