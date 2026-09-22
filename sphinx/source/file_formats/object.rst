Object
======
Defines a collection of models to render together as a single object.
May be separated into multiple LODs (levels of detail).
May also contain additional values used as defaults in the engine at runtine, if not provided by the game.


Render
------

- ``color`` is color in sRGB space
- ``intensity`` is multiplier for the color after it is converted from sRGB to linear
- ``opacity``


Skeletal animation
------------------

- ``name`` name of the animation asset to use by default


Size
----

- ``world`` is the size of the whole object, across its diagonal, in world space units, typically in meters.
- ``pixels`` is the same length but in pixels. This is used to determine LOD at runtime.

The ``threshold`` defines at which point the LOD becomes active.


Lods
----

- ``preload`` is number of levels to automatically preload when this object is loaded. Models in the remaining levels are loaded on demand.


Example ``lods.object``
------------------------

.. code-block:: ini

	#[render]
	#color = 0.4, 0.2, 1
	#intensity = 1
	#opacity = 0.5

	#[skeletalAnimation]
	#name = animationName

	[size]
	world = 2
	pixels = 200

	[lods]
	preload = 2

	[]
	threshold = 1
	lod0.obj

	[]
	threshold = 2
	lod1.obj

	[]
	threshold = 4
	lod2.obj

	[]
	threshold = 8
	lod3.obj
