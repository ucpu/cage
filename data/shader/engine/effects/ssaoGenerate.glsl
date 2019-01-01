
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

const float ssaoRadius = 0.50;
const uint sampleCount = 64;

vec3 s2w(vec2 p, float d)
{
	vec4 p4 = vec4(p, d, 1.0);
	p4 = uniViewProjInv * p4;
	return p4.xyz / p4.w;
}

void main()
{
	vec2 texelSize = float(CAGE_SHADER_SSAO_DOWNSCALE) / textureSize(texNormal, 0).xy;
	vec2 myUv = gl_FragCoord.xy * texelSize;

	// my normal
	vec3 myNormal = textureLod(texNormal, myUv, 0).xyz;
	if (dot(myNormal, myNormal) < 0.1)
	{
		// no lighting here
		outAo = 0.0;
		return;
	}

	// my position
	float myDepth = textureLod(texDepth, myUv, 0).x * 2.0 - 1.0;
	vec3 myPos = s2w(myUv * 2.0 - 1.0, myDepth);

	// sampling
	int n = int(randomFunc(myPos * 10.0) * 10000.0);
	float occ = 0.0;
	float total = 0.0;
	for (int i = 0; i < sampleCount; i++)
	{
		vec3 dir = pointsOnSphere[(n + i) % 256];
		float d = dot(myNormal, dir);
		dir = sign(d) * dir; // move the direction into front hemisphere
		d = abs(d);
		if (d < 0.3)
			continue; // the direction is close to the surface and susceptible to noise
		float r = (randomNumbers[(n * 13 + i * 2) % 256]) * ssaoRadius;
		vec3 sw = myPos + dir * r;
		vec4 s4 = uniViewProj * vec4(sw, 1.0);
		vec3 ss = s4.xyz / s4.w;
		float sampleDepth = textureLod(texDepth, ss.xy * 0.5 + 0.5, 0).x * 2.0 - 1.0;
		if (sampleDepth < ss.z)
		{
			vec3 otherPos = s2w(ss.xy, sampleDepth);
			float diff = length(sw - otherPos);
			occ += smoothstep(1.0, 0.0, diff / ssaoRadius);
		}
		total += 1.0;
	}
	outAo = occ / total;
}