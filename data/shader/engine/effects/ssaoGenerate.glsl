
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

// https://raw.githubusercontent.com/openglsuperbible/sb6code/master/bin/media/shaders/ssao/ssao.fs.glsl

$include ../func/pointsOnSphere.glsl
$include ../func/randomNumbers.glsl
$include ../func/random.glsl

layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texNormal;
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

out float outAo;

const float ssaoRadius = 20.0;
const uint sampleCount = 64;

float linearize(float d)
{
	return 1.0 / (1.0 + d);
}

void main()
{
	vec2 texelSize = 2.0 / textureSize(texNormal, 0).xy;
	vec2 myUv = gl_FragCoord.xy * texelSize;
	//vec3 N = textureLod(texNormal, myUv, 0).xyz;
	float myDepth = linearize(textureLod(texDepth, myUv, 0).x);
	int n = int(randomFunc(gl_FragCoord.xy) * 10000.0);

	float occ = 0.0;
	for (int i = 0; i < sampleCount; i++)
	{
		vec3 dir = pointsOnSphere[(n + i) % 256];
		float r = (randomNumbers[(n * 13 + i) % 256] * 0.9 + 0.1) * ssaoRadius;
		// todo inverse direction if in "back" hemisphere
		float sampleDepth = linearize(textureLod(texDepth, (myUv + dir.xy * r * texelSize), 0).x);
		//sampleDepth += dir.z * r * 0.00001;
		if (myDepth < sampleDepth)
		{
			float d = sampleDepth - myDepth;
			occ += max(1.0 - d * 10.0, 0.0);
		}
	}

	outAo = 1.0 - occ / sampleCount;
	outAo = min(outAo * 2.0, 1.0); // todo remove this temporary workaround
}
