
$include ../shaderConventions.h

$define shader vertex

layout(location = 0) uniform vec4 pos; // x1, y1, x2, y2
layout(location = 1) uniform vec4 uv; // x1, y1, x2, y2
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
out vec2 varUv;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
	gl_Position.xy = pos.xy + (pos.zw - pos.xy) * inPosition.xy;
	varUv = uv.xy + (uv.zw - uv.xy) * inUv;
}

$define shader fragment

in vec2 varUv;
out vec4 outColor;

vec4 delinearize(vec4 ca)
{
#ifdef Delinearize
    return vec4(pow(ca.rgb, vec3(1.0 / 2.2)), ca.a);
#else
	return ca;
#endif
}

vec4 desaturateDisabled(vec4 ca)
{
#ifdef Disabled
	const vec3 lum = vec3(0.299, 0.587, 0.114);
	float bw = dot(lum, vec3(ca)); // desaturate
	bw = (bw - 0.5) * 0.7 + 0.5; // reduce contrast
	return vec4(vec3(bw), ca.a);
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
	outColor = mix(a, b, aniTexFrames.z);
	outColor = delinearize(outColor);
	outColor = desaturateDisabled(outColor);
}

#else

layout(binding = 0) uniform sampler2D texImg;

void main()
{
	outColor = texture(texImg, varUv);
	outColor = delinearize(outColor);
	outColor = desaturateDisabled(outColor);
}

#endif
