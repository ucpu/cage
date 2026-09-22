Math
====

.. code-block:: cpp

	Vec3 a = {1, 2, 3};
	Vec3 b = Vec3(10);
	Vec3 c = a + b * 0.5;
	CAGE_LOG(SeverityEnum::Info, "example", Stringizer() + c); // logs (6,7,8)
