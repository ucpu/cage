print('cage-error')
exit()


import bpy
import struct
import string
from array import array
from sys import stdin
import json
import cage

# global configuration
gExportUV = False
gExportNormals = False
gExportSkeleton = False
gGenerateIndicies = False
gGenerateTangents = False

# global bones order
gOrder = []

# serialize attributes arrays
def serializeArrays(mesh):
	attributes = {'vertices': []}

	# add normals
	if(gExportNormals):
		attributes['normals'] = []

	# add texture coordinates 
	if(gExportUV):
		attributes['texCoords'] = []

	# add bones and weights
	if(gExportSkeleton):
		attributes['bones'] = []
		attributes['weights'] = []

	nPoly = 0
	# for each polygon (triangle)
	for i in mesh.polygons:
		# for each verts
		nVert = 0
		for j in i.vertices:
			pos = mesh.vertices[j].co.xyz

			# append as tupples
			# vertices
			attributes['vertices'].append((pos[0], pos[1], pos[2]))
#			attributes['vertices'].append((pos[0], pos[2], -pos[1]))
			if(gExportNormals):
				normal = mesh.vertices[j].normal
				attributes['normals'].append((normal[0], normal[2], -normal[1]))
			# texture coordinates
			if(gExportUV):
				uv = mesh.uv_layers[0].data[nPoly * 3 + nVert].uv.xy
				attributes['texCoords'].append((uv[0], uv[1]))
			# groups and weights
			if(gExportSkeleton):
				groups = tuple(gOrder[g.group] for g in mesh.vertices[j].groups)
				weights = tuple(g.weight for g in mesh.vertices[j].groups)
				groups += tuple(0 for dummy in range(len(groups), 4))
				weights += tuple(0.0 for dummy in range(len(weights), 4))
				attributes['bones'].append(groups)
				attributes['weights'].append(weights)
			nVert += 1
		nPoly += 1

	return attributes

# make indicies and transform attributes arrays equaly
def makeIndicies(attributes):
	exp = {'vertices':[], 'indicies':[]}

	# add normals
	if(gExportNormals):
		exp['normals'] = []
			
	# add textures coordinates
	if(gExportUV):
		exp['texCoords'] = []

	# add bones and weights
	if(gExportSkeleton):
		exp['bones'] = []
		exp['weights'] = []

	indiciesId = 0
	i = 0
	attributeLen = len(attributes['vertices'])
	# for each tupple in attributes
	while i < attributeLen:
		found = -1;
		j = 0
		expLen = len(exp['vertices'])
		# for each tupple in exp
		while j < expLen:
			if(attributes['vertices'][i] == exp['vertices'][j] and (not gExportUV or attributes['texCoords'][i] == exp['texCoords'][j]) and (not gExportNormals or attributes['normals'][i] == exp['normals'][j])):
				found = j
				break
			j += 1
			
		# new attribute
		if(found == -1):
			for key, att in attributes.items():
				exp[key].append(attributes[key][i])
			exp['indicies'].extend([indiciesId])
			indiciesId = indiciesId + 1
		# already created attribute
		else:
			exp['indicies'].append(found)
		
		i += 1
	return exp

# make attributes array from to_mesh() format  
def makeAttributes(mesh):
	attributes = serializeArrays(mesh)
	
	if(gGenerateIndicies):
		exp = makeIndicies(attributes)
		
		# flatten
		for key, att in exp.items():
			if(key != 'indicies'):
				exp[key] = list(sum(att, ()))
	else:
		attributes['indicies'] = []
		return attributes

	return exp

#############################################################
#                     SCRIPT START
#############################################################

# read base configuration - first 6 lines
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
gExportUV = cage.checkConfiguration(configuration, 'export_uv')
gExportNormals = cage.checkConfiguration(configuration, 'export_normal')
gGenerateIndicies = cage.checkConfiguration(configuration, 'export_indices')
gGenerateTangents = cage.checkConfiguration(configuration, 'export_tangents')
gExportSkeleton = cage.checkConfiguration(configuration, 'export_skeleton')

# open file
bpy.ops.wm.open_mainfile(filepath = resourcesPath + '/' + assetName[0])

# transform mesh if exist
if(bpy.data.meshes.find(assetName[1]) != -1):
	# geometry
	mesh = bpy.data.meshes[assetName[1]]

	# find skeleton structure
	if(gExportSkeleton):
		for i in bpy.data.objects:
			if(i.type == 'MESH' and i.data.name == assetName[1]):
				order = ['Root']
				order.extend([j.name for j in i.modifiers['Armature'].object.data.bones['Root'].children_recursive])
				for g in i.vertex_groups:
					gOrder.append(order.index(g.name))
				break;
		# debug bone order
		#print('Mesh:', order)
		#print('Mesh:', gOrder)

	# attributes
	mesh = makeAttributes(mesh)
	
	# header
	mesh['flags'] = (1 if gExportUV else 0) + (2 if gExportNormals else 0) + (4 if gGenerateTangents else 0) + (8 if gExportSkeleton else 0)
	mesh['verticesLen'] = len(mesh['vertices']) // 3
	mesh['indiciesLen'] = len(mesh['indicies'])
	mesh['skeletonScheme'] = int(configuration['scheme_skeleton'])
	mesh['skeleton'] = 0 #todo

	# debug output
	if(cage.checkConfiguration(configuration, 'debug_json')):
		with open(assetsPath + '/' + assetHash + '_' + assetName[1] + '.json', mode='w') as file:
			json.dump(mesh, file, indent=2, separators=(',',':'))

	# output
	with open(assetsPath + '/' + assetHash, mode='bw') as file:
		file.write(struct.pack('=I', mesh['flags']))
		file.write(struct.pack('=I', mesh['verticesLen']))
		file.write(struct.pack('=I', mesh['indiciesLen']))
		file.write(struct.pack('=I', mesh['skeletonScheme']))
		file.write(struct.pack('=I', mesh['skeleton']))

		# attributes
		array('f', mesh['vertices']).tofile(file)
		if(gExportUV):
			array('f', mesh['texCoords']).tofile(file)
		if(gExportNormals):
			array('f', mesh['normals']).tofile(file)
		if(gGenerateTangents):
			array('f', mesh['tangents']).tofile(file)			
			array('f', mesh['bitangents']).tofile(file)			
		if(gExportSkeleton):
			array('H', mesh['bones']).tofile(file)			
			array('f', mesh['weights']).tofile(file)

		# indices
		if(gGenerateIndicies):
			array('I', mesh['indicies']).tofile(file)
		
		# material #todo

print('cage-begin')
print('use=' + assetName[0])
print('cage-end')

