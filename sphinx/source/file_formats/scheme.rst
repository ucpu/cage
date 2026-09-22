Scheme
======
Defines properties that can be configured for asset processor for a specific type of asset.


Scheme section
--------------
Mandatory section that defines how to convert assets of this type.

- ``processor`` is program name with parameters to invoke for the conversion.
- ``index`` is the index that defines which scheme to use for loading the converted assets in the engine at runtime


Properties
----------
All other sections describe a property that will be provided to the converter.

- ``display`` is name of the property to be displayed in graphical programs
- ``hint`` is a tooltip that will be shown for the property in graphical programs
- ``type`` one of: bool, sint32, uint32, real, string, enum
- ``default`` is value that will be provided to the converter in case the scheme file does not define it
- ``values`` is list of accepted values for enum type, separated by comma
- ``min`` and ``max`` defines applicable range for numerical types


Example ``sound.scheme``
------------------------

.. code-block:: ini

	[scheme]
	processor = cage-asset-processor sound
	index = 20

	[gain]
	display = gain
	type = real
	min = 0
	default = 1.0

	[sampleRate]
	display = sample rate
	hint = use 0 to keep original from the input file
	type = uint32
	default = 48000

	[mono]
	display = mono channel
	hint = convert to mono channel (required for spatial audio)
	type = bool
	default = true
