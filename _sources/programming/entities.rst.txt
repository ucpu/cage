Entities
========


Manager
-------

.. code-block:: cpp

	struct PositionComponent
	{
		Vec3 position;
	};

	Holder<EntityManager> manager = newEntityManager();
	manager->defineComponent(PositionComponent());


Operating on entities
---------------------

.. code-block:: cpp

	Entity *e = manager->createUnique();
	e->value<PositionComponent>().position = Vec3(10, 20, 30); // implicitly adds the component to the entity
	e->remove<PositionComponent>();


Visiting entities
-----------------

.. code-block:: cpp

	Vec3 sum;
	uint32 count = 0;
	entitiesVisitor([&](Entity *e, const PositionComponent &pos){
		sum += pos.position;
		count++;
	}, manager, false);
	const Vec3 avg = sum / count;
