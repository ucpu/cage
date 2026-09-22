Assets
======
Defines list of assets and their properties for conversion.


Format
------
Each section must contain the ``scheme`` key, with the name of a scheme file that defines how to convert assets in this section.
It may contain any number of additional properties, as defined in the scheme file.
It may contain multiple asset names to convert with the same set of properties.


Example ``scene.assets``
------------------------

.. code-block:: ini

	[]
	scheme = model
	material = amber.cpm
	scale = 50
	axes = +x-z+y
	MosquitoInAmber.glb?Amber

	[]
	scheme = model
	material = mosquito.cpm
	scale = 50
	axes = +x-z+y
	MosquitoInAmber.glb?Mosquito

	[]
	scheme = model
	bones = true
	turtle.glb?BackFlippers
	turtle.glb?Eyes
	turtle.glb?Shell
	turtle.glb?UpperBody

	[]
	scheme = skeleton
	turtle.glb;skeleton

	[]
	scheme = animation
	turtle.glb?Swim

	[]
	scheme = texture
	srgb = true
	turtle.glb?BackFlippers_albedo
	turtle.glb?Shell_albedo
	turtle.glb?UpperBody_albedo

	[]
	scheme = texture
	turtle.glb?BackFlippers_special
	turtle.glb?Shell_special
	turtle.glb?UpperBody_special

	[]
	scheme = object
	MosquitoInAmber.object
	turtle.object

	[]
	scheme = pack
	scene.pack
