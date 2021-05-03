#include "main.h"

#include <cage-core/entitiesVisitor.h>
#include <cage-core/math.h>

void testEntitiesVisitor()
{
	CAGE_TESTCASE("entities visitor");

	Holder<EntityManager> man = newEntityManager();

	man->defineComponent(vec3());
	man->defineComponent(real());
	man->defineComponent(uint32());

	Entity *a = man->createUnique();
	Entity *b = man->createUnique();
	Entity *c = man->createUnique();

	a->value<vec3>() = vec3(3);
	a->value<real>() = 3;
	a->value<uint32>() = 3;

	b->value<vec3>() = vec3(4);
	b->value<real>() = 4;

	c->value<vec3>() = vec3(5);
	c->value<uint32>() = 5;

	visit(+man, [](vec3 &v, real &r) { v[0] *= r; });
	visit(+man, [](vec3 &v, uint32 &u) { v[1] *= u; });
	visit(+man, [](vec3 &v, real &r, uint32 &u) { v[2] = 0; });

	CAGE_TEST(a->value<vec3>() == vec3(9, 9, 0));
	CAGE_TEST(b->value<vec3>() == vec3(16, 4, 4));
	CAGE_TEST(c->value<vec3>() == vec3(5, 25, 5));
}

