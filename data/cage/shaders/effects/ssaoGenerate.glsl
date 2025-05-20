
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

$include ../functions/common.glsl
$include ../functions/randomFunc.glsl
$include ssaoParams.glsl

layout(std140, binding = 3) uniform SsaoPoints
{
	vec4 ssaoPoints[256];
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
	return normalize(cross(dFdxFine(position), dFdyFine(position)));
}

mat3 makeTbn(vec3 myNormal)
{
	//float angle = 2.0 * 3.14159265 * fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
	uint n = hash(uint(gl_FragCoord.x) ^ hash(uint(gl_FragCoord.y)) ^ uint(iparams[1])); // per-pixel seed
	float angle = 2.0 * 3.14159265 * egacIntToFloat(n);
	float ca = cos(angle);
	float sa = sin(angle);
	vec3 t = normalize(myNormal.yzx); // arbitrary orthogonal vector
	vec3 b = cross(myNormal, t);
	vec3 tangent = ca * t + sa * b;
	vec3 bitangent = -sa * t + ca * b;
	return mat3(tangent, bitangent, myNormal);
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
	mat3 tbn = makeTbn(myNormal);
	float raysLength = params[3];
	float occ = 0;
	float total = 0;
	for (int i = 0; i < iparams[0]; i++)
	{
		// view-space ray direction
		vec3 rayDir = ssaoPoints[i].xyz;
		rayDir = tbn * rayDir;
		rayDir *= raysLength;

		// view-space ray position
		vec3 rayPos = myPos + rayDir;
		vec4 r4 = proj * vec4(rayPos, 1); // clip space
		vec3 rayNdc = r4.xyz / r4.w; // NDC [-1,1]

		vec2 sampleUv = saturate(rayNdc.xy * 0.5 + 0.5);
		float sampleDepth = textureLod(texDepth, sampleUv, 0).x * 2 - 1; // NDC depth [-1,1]
		if (sampleDepth < rayNdc.z)
		{
			vec3 samplePos = ndcToView(rayNdc.xy, sampleDepth); // view-space sample position
			float diff = length(rayPos - samplePos);
			occ += smoothstep(1, 0, saturate(diff / raysLength));
		}
		total += 1;
	}

	if (total > 0)
		outAo = occ / total;
	else
		outAo = 0;
}
