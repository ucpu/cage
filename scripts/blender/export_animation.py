print('cage-error')
exit()

import bpy
import struct
import string
from array import array
from sys import stdin
import json
import cage

# transform quaternion - rotation animation
def transformQuat(animation, start, end):
	d = end - start
	frames = len(animation) // 4
	quats = []
	times = []
	i = 0
	while(i < frames):
		times.append((animation[i]['t'] - start) / d)
		quats.extend([animation[i + j * frames]['v'] for j in [1,2,3,0]])
		i += 1

	return (quats, times)

#transform location - location animation
def transformPos(animation, start, end):
	d = end - start
	frames = len(animation) // 3
	vecs = []
	times = []
	i = 0
	while(i < frames):
		times.append((animation[i]['t'] - start) / d)
		vecs.extend([animation[i + j * frames]['v'] for j in [0,1,2]])
		i += 1

	return (vecs, times)

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
cage.printConfiguration(configuration)
print('Animation: ', assetHash)

# open file
bpy.ops.wm.open_mainfile(filepath = resourcesPath + '/' + assetName[0])

print(assetName[1])

# transform armature if exist
if(bpy.data.actions.find(assetName[1]) != -1):
	action = bpy.data.actions[assetName[1]]

	# find skeleton structure
	for i in bpy.data.objects:
		if(i.type == 'ARMATURE' and i.animation_data.action == action):
			order = ['Root']
			order.extend([j.name for j in i.data.bones['Root'].children_recursive])
			print('Animation:', order)
			break;

	index = 0
	indexes = []
	locationFramesLens = []
	rotationFramesLens = []
	groups = []
	size = 0
	
	# transform
	for i in order:
		if(action.groups.find(i) == -1):
			continue
		
		group = action.groups[i]
		indexes.append(index)
		index += 1
		animationQuat = []
		animationPos = []
		
		for channel in group.channels:
			if(channel.data_path.find('rotation_quaternion') != -1):
				for key in channel.keyframe_points:
					animationQuat.append({'t': key.co.x, 'v': key.co.y})
			elif(channel.data_path.find('location') != -1):
				for key in channel.keyframe_points:
					animationPos.append({'t': key.co.x, 'v': key.co.y})
			else:
				print('Ignore channel', channel.data_path)

		# size
		rotationFramesLens.append(len(animationQuat) // 4)
		locationFramesLens.append(len(animationPos) // 3)

		# frames
		g = {}
		g['rotationFrames'], g['rotationFramesTimes'] = transformQuat(animationQuat, action.frame_range[0], action.frame_range[1])
		g['positionFrames'], g['positionFramesTimes'] = transformPos(animationPos, action.frame_range[0], action.frame_range[1])
		groups.append(g)
		size += (len(g['positionFrames']) + len(g['positionFramesTimes']) + len(g['rotationFrames']) + len(g['rotationFramesTimes'])) * 4

	# output
	with open(assetsPath + '/' + assetHash, mode='bw') as file:
		# duration
		file.write(struct.pack('=Q', int((action.frame_range[1] - action.frame_range[0]) * 1000000.0 / 24.0)))
		# bones
		file.write(struct.pack('=I', len(groups)))
		# size
		size += (len(indexes) + len(locationFramesLens) + len(rotationFramesLens)) * 2
		file.write(struct.pack('=I', size))
		# indexes
		array('H', indexes).tofile(file)
		# position frames size
		array('H', locationFramesLens).tofile(file)
		# rotation frames size
		array('H', rotationFramesLens).tofile(file)

		# for each group
		for g in groups:
			array('f', g['positionFramesTimes']).tofile(file)
			array('f', g['positionFrames']).tofile(file)
			array('f', g['rotationFramesTimes']).tofile(file)
			array('f', g['rotationFrames']).tofile(file)

	# debug output
	if(cage.checkConfiguration(configuration, 'debug_json')):
		with open(assetsPath + '/' + assetHash + '_' + assetName[1] + '.json', mode='w') as file:
			json.dump({
				'duration': int((action.frame_range[1] - action.frame_range[0]) * 1000000.0 / 24.0),
				'boneGroups': len(groups),
				'size': size,
				'indexes': indexes,
				'positionFrames': locationFramesLens,
				'rotationFrames': rotationFramesLens,
				'groups': groups
			}, file, indent=2, separators=(',',':'))

print('cage-begin')
print('use=' + assetName[0])
print('cage-end')


