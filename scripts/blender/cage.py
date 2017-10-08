from sys import stdin

# print configuration - for debug purposes only
def printConfiguration(configuration):
	for key, c in configuration.items():
		print(key, c)

# check if given name is in the configuration and if it's true
def checkConfiguration(configuration, name):
	return name in configuration and configuration[name] == 'true'

#read configuration from stdin
def readConfiguration():
	configuration = {}
	while(1):
		line = stdin.readline()[:-1]
		if(line == 'cage-end'):
			break;
		line = line.split('=')
		configuration[line[0]] = line[1]

	return configuration

#def hashString(str):
#	import ctypes
#	
#	GCHL_HASH_OFFSET = ctypes.c_uint32(2166136261)
#	GCHL_HASH_PRIME = ctypes.c_uint32(16777619)
#	
#	bstr = ctypes.c_char_p(str.encode())
#
#	hash = GCHL_HASH_OFFSET
#	while(str != ''):
#		hash = hash * GCHL_HASH_PRIME
#		break
#
#	return hash
#
