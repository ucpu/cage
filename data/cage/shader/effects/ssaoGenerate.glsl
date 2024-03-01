
$include vertex.glsl

$define shader fragment

$include ../functions/hash.glsl
$include ssaoParams.glsl

layout(std140, binding = 3) uniform SsaoPoints
{
	vec4 pointsOnSphere[256];
};

layout(binding = 0) uniform sampler2D texDepth;

out float outAo;

vec3 reconstructNormal(vec3 position)
{
	return normalize(cross(dFdx(position), dFdy(position)));
}

vec3 s2w(vec2 p, float d)
{
	vec4 p4 = vec4(p, d, 1);
	p4 = projInv * p4;
	return p4.xyz / p4.w;
}

float sqr(float a)
{
	return a * a;
}

void main()
{
	vec2 resolution = vec2(textureSize(texDepth, 0));
	vec2 myUv = gl_FragCoord.xy / resolution;
	float myDepth = textureLod(texDepth, myUv, 0).x * 2.0 - 1.0;
	if (myDepth > 0.999)
	{ // no occlusion on skybox
		outAo = 0;
		return;
	}
	vec3 myPos = s2w(myUv, myDepth);
	vec3 myNormal = reconstructNormal(myPos);

	// sampling
	int n = int(hash(uint(gl_FragCoord.x) ^ hash(uint(gl_FragCoord.y)) ^ iparams[1]));
	float ssaoRadius = params[3];
	float occ = 0.0;
	float total = 0.0;
	for (int i = 0; i < iparams[0]; i++)
	{
		vec3 dir = pointsOnSphere[hash(n * 17 + i) % 256].xyz;
		float d = dot(myNormal, dir);
		if (abs(d) < 0.1)
			continue; // the direction is close to the surface and susceptible to noise
		dir = sign(d) * dir; // move the direction into front hemisphere
		float r = (sqr(pointsOnSphere[hash(n * 13 + i) % 256].w) * 0.9 + 0.1) * ssaoRadius;
		vec3 sw = myPos + dir * r;
		vec4 s4 = proj * vec4(sw, 1.0);
		vec3 ss = s4.xyz / s4.w;
		float sampleDepth = textureLod(texDepth, ss.xy, 0).x * 2.0 - 1.0;
		if (sampleDepth < ss.z)
		{
			vec3 otherPos = s2w(ss.xy, sampleDepth);
			float diff = length(sw - otherPos);
			occ += smoothstep(1.0, 0.0, diff / ssaoRadius);
		}
		total += 1.0;
	}
	if (total > 0)
		outAo = occ / total;
	else
		outAo = 0;
}
