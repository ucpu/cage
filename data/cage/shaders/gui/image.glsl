
layout(std430, set = 2, binding = 0) readonly buffer Global
{
	vec4 uniPos; // x1, y1, x2, y2
	vec4 uniUv; // x1, y1, x2, y2
	vec4 uniAnimation; // time (seconds), speed, offset (normalized), unused
};


layout(location = 0) varying vec2 varUv;


$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inUv;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
	gl_Position.xy = uniPos.xy + (uniPos.zw - uniPos.xy) * inPosition.xy;
	varUv = uniUv.xy + (uniUv.zw - uniUv.xy) * inUv;
}


$define shader fragment

$include ../functions/sampleTextureAnimation.glsl

layout(location = 0) out vec4 outColor;

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

layout(set = 2, binding = 1) uniform sampler2DArray texImg;

void main()
{
	outColor = sampleTextureAnimation(texImg, varUv, uniAnimation, vec4(1, 0, 0, 0));
	outColor = delinearize(outColor);
	outColor = desaturateDisabled(outColor);
}

#else

layout(set = 2, binding = 1) uniform sampler2D texImg;

void main()
{
	outColor = texture(texImg, varUv);
	outColor = delinearize(outColor);
	outColor = desaturateDisabled(outColor);
}

#endif
