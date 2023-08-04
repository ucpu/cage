
$include ../shaderConventions.h

$define shader vertex

layout(location = 0) uniform vec4 pos;
layout(location = 1) uniform vec4 uv;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
out vec2 varUv;

void main()
{
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
	gl_Position.xy = pos.xy + (pos.zw - pos.xy) * inPosition.xy;
	varUv = uv.xy + (uv.zw - uv.xy) * inUv;
}

$define shader fragment

in vec2 varUv;
out vec4 outColor;

vec4 desaturateDisabled(vec4 ca)
{
#ifdef Disabled
	const vec3 lum = vec3(0.299, 0.587, 0.114);
	float bw = dot(lum, vec3(ca));
	return vec4(bw, bw, bw, ca.a);
#else
	return ca;
#endif
}

#ifdef Animated

layout(location = 2) uniform vec4 aniTexFrames;
layout(binding = 0) uniform sampler2DArray texImg;

void main()
{
	vec4 a = texture(texImg, vec3(varUv, aniTexFrames.x));
	vec4 b = texture(texImg, vec3(varUv, aniTexFrames.y));
	outColor = desaturateDisabled(mix(a, b, aniTexFrames.z));
}

#else

layout(binding = 0) uniform sampler2D texImg;

void main()
{
	outColor = desaturateDisabled(texture(texImg, varUv));
}

#endif
