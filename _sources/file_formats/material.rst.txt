Material
========
Typically uses .cpm extension (Cage Pbr Material).


Example ``amber.cpm``
---------------------

.. code-block:: ini

	[textures]
	albedo = amber_albedo.png
	special = amber_special.png
	normal = amber_normal.png

	[render]
	shader = amber.glsl


Example ``everything.cpm``
--------------------------

.. code-block:: ini

	[base]
	albedo = 0, 0, 0
	intensity = 1
	opacity = 0
	roughness = 0
	metallic = 0
	emission = 0
	mask = 0

	[mult]
	albedo = 1, 1, 1
	intensity = 1
	opacity = 1
	roughness = 1
	metallic = 1
	emission = 1
	mask = 1

	[textures]
	albedo = albedo.png
	special = special.png
	normal = normal.png
	custom = custom.png

	[render]
	shader = standard.glsl
	layer = 0

	[animation]
	duration = 1
	loop = true

	[flags]
	cutOut
	transparent
	fade
	orderIndependent
	twoSided
	noDepthTest
	noDepthWrite
	noLighting
	noShadowCast
	noCulling
