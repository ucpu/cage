print('cage-error')
exit()

import bpy
import struct
import string
from array import array
from sys import stdin
import json
import cage
import mathutils

# global configuration
gExportAnimations = False
gExportNames = False

# transform column major to row major
def rowMajor(matrix):
	newMatrix = matrix[::4]
	newMatrix.extend(matrix[1::4])
	newMatrix.extend(matrix[2::4])
	newMatrix.extend(matrix[3::4])
	return newMatrix

def makeRestInverted(matrix):
	matrix = matrix.inverted()
	return rowMajor([j for i in matrix for j in i])

def makeBaseMatrix(bone):
	if(bone.parent != None):
		matrix = bone.parent.matrix_local.inverted() * bone.matrix_local
	else:
		matrix = bone.matrix_local
	return rowMajor([j for i in matrix for j in i])

#############################################################
#                     SCRIPT START
#############################################################

# read base configuration - first 7 lines
resourcesPath = stdin.readline()[:-1]
assetName = stdin.readline()[:-1].split('?')
assetsPath = stdin.readline()[:-1]
assetHash = stdin.readline()[:-1]
assetConfig = stdin.readline()[:-1]
scheme = stdin.readline()[:-1]

# read configuration for asset - n lines
configuration = cage.readConfiguration()

# print configuration
#cage.printConfiguration(configuration)

# set global configuration
gExportAnimations = cage.checkConfiguration(configuration, 'export_animations')
gExportNames = cage.checkConfiguration(configuration, 'export_names')

# open file
bpy.ops.wm.open_mainfile(filepath = resourcesPath + '/' + assetName[0])

# transform armature if exist
if(bpy.data.armatures.find(assetName[1]) != -1):
	# list of sorted bones objects
	bonesRecursive = [bpy.data.armatures[assetName[1]].bones['Root']]
	bonesRecursive.extend(bonesRecursive[0].children_recursive)

	# debug bone order
	print('Skeleton:', [i.name for i in bonesRecursive])
		
	lenBones = len(bonesRecursive)

	# output
	with open(assetsPath + '/' + assetHash, mode='bw') as file:
		# number of bones
		file.write(struct.pack('=I', lenBones))

		# todo connect points
		file.write(struct.pack('=I', 0))
		
		# todo animation
		file.write(struct.pack('=I', 0))

		# scheme
		file.write(struct.pack('=I', int(configuration['scheme_animation'])))

		boneNames = []
		parents = []
		baseMatrices = []
		restMatrices = []

		for bone in bonesRecursive:
			boneNames.append(bone.name)
			parents.append(boneNames.index(bone.parent.name) if bone.parent != None else 65535)
			baseMatrices.extend(makeBaseMatrix(bone))
			restMatrices.extend(makeRestInverted(bone.matrix_local))
			
		# parents
		array('H', parents).tofile(file)
		# base matrices - relative to parent
		array('f', baseMatrices).tofile(file)
		# inverted rest matrices
		array('f', restMatrices).tofile(file)

	# debug output
	if(cage.checkConfiguration(configuration, 'debug_json')):
		with open(assetsPath + '/' + assetHash + '_' + assetName[1] + '.json', mode='w') as file:
			json.dump({
				'bones': lenBones,
				'names': 0,
				'animations': 0,
				'scheme': int(configuration['scheme_animation']),
				'parennts': parents,
				'baseMatrices': baseMatrices,
				'restMatrices': restMatrices
			}, file, indent=2, separators=(',',':'))

print('cage-begin')
print('use=' + assetName[0])
print('cage-end')

