Ini
===
This format is used for most text files in Cage.

It is slightly modified from the original ini.


Basic example
-------------

.. code-block:: none

	# hash character is used to start a comment

	[skinning] # section name
	level = 3 # key/value pairs
	experience = 12000

	[skinning/levels]
	linenCloth = 40
	leatherCloth = 13


Advanced example
----------------
This example focuses on changes from the original format.

.. code-block:: none

	[] # empty section name is replaced with numerical index, 0 in this case
	aaa # key/value pair without = has the key replaced by numerical index, 0 in this case
	bbb # 1 = bbb

	[] # 1
	ccc # numerical keys in different sections restart from 0

	[] # 2
	key = value # it is possible to mix named keys and unnamed, but be careful with numerical keys that may collide with the unnamed keys
	ddd # 0

	[] # 3
	 key      =      value    # names for both the key and value are trimmed
	Gandalf the Gray = human wizard # names may contain spaces in the middle, no escaping
	equal sign = == # values may contain equal sign, no escaping, but key cannot contain equal sign
	== # this is anonymous key with value of equal sign

	[specialCharacters]
	&
	" # this is ok, no escaping
	'
	# this is just empty line, cannot have # in key nor in value
	= # this is unnamed key with empty value

	[chemicalBonds]
	c-c
	= c=c # make sure to use = to separate key from value
