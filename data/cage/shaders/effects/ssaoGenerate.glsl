
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

$include ../functions/common.glsl
$include ../functions/hash.glsl
//$include ssaoParams.glsl


layout(std140, binding = CAGE_SHADER_UNIBLOCK_CUSTOMDATA) uniform Ssao
{
	mat4 proj;
	mat4 projInv;
	vec4 params; // strength, bias, power, radius
	ivec4 iparams; // sampleCount, hashSeed
};


layout(std140, binding = 3) uniform SsaoPoints
{
	// unit sphere samples; .w is [0,1] random scalar
	vec4 pointsOnSphere[256];
};

layout(binding = 0) uniform sampler2D texDepth; // NDC depth [0,1]

out float outAo;

// inputs in NDC [-1,1]
vec3 ndcToView(vec2 p, float d)
{
	vec4 p4 = vec4(p, d, 1);
	p4 = projInv * p4;
	return p4.xyz / p4.w;
}

// all view-space
vec3 reconstructNormal(vec3 position)
{
	return normalize(cross(dFdx(position), dFdy(position)));
}

void main()
{
	vec2 resolution = vec2(textureSize(texDepth, 0));
	vec2 myUv = gl_FragCoord.xy / resolution; // screen-space [0,1]
	float myDepth = textureLod(texDepth, myUv, 0).x * 2 - 1; // NDC depth [-1,1]
	if (myDepth > 0.999)
	{ // no occlusion on skybox
		outAo = 0;
		return;
	}
	vec3 myPos = ndcToView(myUv * 2 - 1, myDepth); // view-space position
	vec3 myNormal = reconstructNormal(myPos); // view-space normal

	// sampling
	uint n = hash(uint(gl_FragCoord.x) ^ hash(uint(gl_FragCoord.y)) ^ uint(iparams[1])); // per-pixel seed
	float ssaoRadius = params[3];
	float occ = 0;
	float total = 0;
	for (int i = 0; i < iparams[0]; i++)
	{
		// view-space ray direction
		vec3 rayDir = pointsOnSphere[hash(uint(n * 17 + i)) % 256].xyz;
		float d = dot(myNormal, rayDir);
		if (abs(d) < 0.1)
			continue; // the direction is close to the surface and susceptible to noise
		rayDir = sign(d) * rayDir; // move the direction into front hemisphere
		rayDir *= (sqr(pointsOnSphere[hash(uint(n * 13 + i)) % 256].w) * 0.9 + 0.1) * ssaoRadius;

		vec3 rayPos = myPos + rayDir; // view-space ray position
		vec4 r4 = proj * vec4(rayPos, 1); // clip space
		vec3 rayNdc = r4.xyz / r4.w; // NDC [-1,1]

		float sampleDepth = textureLod(texDepth, rayNdc.xy * 0.5 + 0.5, 0).x * 2 - 1; // NDC depth [-1,1]
		if (sampleDepth < rayNdc.z)
		{
			vec3 samplePos = ndcToView(rayNdc.xy, sampleDepth); // view-space sample position
			float diff = length(rayPos - samplePos);
			occ += smoothstep(1, 0, diff / ssaoRadius);
		}
		total += 1;
	}

	if (total > 0)
		outAo = occ / total;
	else
		outAo = 0;
}
