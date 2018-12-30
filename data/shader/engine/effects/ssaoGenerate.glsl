
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

$include ../func/pointsOnSphere.glsl
$include ../func/randomNumbers.glsl
$include ../func/random.glsl

layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texNormal;
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

out float outAo;

layout(location = 0) uniform mat4 uniViewProj;
layout(location = 1) uniform mat4 uniViewProjInv;

const float ssaoRadius = 0.30;
const uint sampleCount = 32;

void main()
{
	vec2 texelSize = float(CAGE_SHADER_SSAO_DOWNSCALE) / textureSize(texNormal, 0).xy;
	vec2 myUv = gl_FragCoord.xy * texelSize;

	// my normal
	vec3 myNormal = textureLod(texNormal, myUv, 0).xyz;
	if (dot(myNormal, myNormal) < 0.1)
	{
		// no lighting here
		outAo = 1.0;
		return;
	}

	// my position
	float myDepth = textureLod(texDepth, myUv, 0).x * 2.0 - 1.0;
	vec4 pos4 = vec4(myUv * 2.0 - 1.0, myDepth, 1.0);
	pos4 = uniViewProjInv * pos4;
	vec3 myPos = pos4.xyz / pos4.w;

	// sampling
	int n = int(randomFunc(myPos) * 10000.0);
	float occ = 0.0;
	for (int i = 0; i < sampleCount; i++)
	{
		vec3 dir = pointsOnSphere[(n + i) % 256];
		float r = (randomNumbers[(n * 13 + i * 2) % 256]) * ssaoRadius;
		dir = sign(dot(myNormal, dir)) * dir;
		vec3 sw = myPos + dir * r;
		vec4 s4 = uniViewProj * vec4(sw, 1.0);
		vec3 ss = s4.xyz / s4.w;
		float sampleDepth = textureLod(texDepth, ss.xy * 0.5 + 0.5, 0).x * 2.0 - 1.0;
		if (sampleDepth + 0.0001 < ss.z)
		{
			//vec4 t4 = vec4(ss.xy, sampleDepth, 1.0);
			//t4 = uniViewProjInv * t4;
			//vec3 tw = t4.xyz / t4.w;
			//float d = expectedDepth - theirDepth;
			//occ += 1.0 / (1.0 + d);
			occ += 1.0;
		}
	}
	outAo = occ / sampleCount;
}
