
$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texLuminance;

layout(location = 0) uniform vec4 uniLuminanceParams; // key, strength, nightOffset, nightScale

out vec3 outColor;

vec3 blueShift(vec3 color, float strength)
{
	const vec3 kRGBToYPrime = vec3(0.299, 0.587, 0.114);
	const vec3 kRGBToI = vec3(0.596, -0.275, -0.321);
	const vec3 kRGBToQ = vec3(0.212, -0.523, 0.311);
	const vec3 kYIQToR = vec3(1.0, 0.956, 0.621);
	const vec3 kYIQToG = vec3(1.0, -0.272, -0.647);
	const vec3 kYIQToB = vec3(1.0, -1.107, 1.704);
	float YPrime = dot(color, kRGBToYPrime);
	float I = dot(color, kRGBToI);
	float Q = dot(color, kRGBToQ);
	float hue = atan(Q, I);
	float chroma = sqrt(I * I + Q * Q);
	hue = mix(hue, 0.7 * 3.14159, strength);
	Q = chroma * sin(hue);
	I = chroma * cos(hue);
	vec3 yIQ = vec3(YPrime, I, Q);
	return vec3(dot(yIQ, kYIQToR), dot(yIQ, kYIQToG), dot(yIQ, kYIQToB));
}

vec3 desaturate(vec3 color, float strength)
{
	float i = dot(color, vec3(0.3, 0.59, 0.11)); // gray intensity
	return mix(color, vec3(i), strength);
}

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;
	float avgLuminanceLog = texelFetch(texLuminance, ivec2(0), 0).x;
	float avgLuminance = exp(avgLuminanceLog);
	vec3 c = color * uniLuminanceParams[0] / avgLuminance;
	float night = clamp((-avgLuminanceLog - uniLuminanceParams[2]) * uniLuminanceParams[3], 0.0, 1.0);
	//c = blueShift(c, night);
	c = desaturate(c, night);
	outColor = mix(color, c, uniLuminanceParams[1]);
}
