
$include ../../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS) uniform sampler2D texLuminance;

layout(std140, binding = CAGE_SHADER_UNIBLOCK_FINALSCREEN) uniform FinalScreen
{
	vec4 tonemapFirst; // shoulderStrength, linearStrength, linearAngle, toeStrength
	vec4 tonemapSecond; // toeNumerator, toeDenominator, white, tonemapEnabled
	vec4 luminanceParams; // key, strength
	vec4 gammaParams; // gamma
};

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

vec3 Uncharted2Tonemap(vec3 x)
{
	float shoulderStrength = tonemapFirst[0];
	float linearStrength = tonemapFirst[1];
	float linearAngle = tonemapFirst[2];
	float toeStrength = tonemapFirst[3];
	float toeNumerator = tonemapSecond[0];
	float toeDenominator = tonemapSecond[1];
	return ((x * (shoulderStrength * x + linearAngle * linearStrength) + toeStrength * toeNumerator) / (x * (shoulderStrength * x + linearStrength) + toeStrength * toeDenominator)) - toeNumerator / toeDenominator;
}

void main()
{
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;

	// eye adaptation (exposure control)
	if (luminanceParams[1] > 0.0)
	{
		float avgLuminance = texelFetch(texLuminance, ivec2(0), 0).x;
		avgLuminance = exp(avgLuminance);
		float li = luminanceParams[0] / avgLuminance;
		vec3 c = color * li;
		//float night = max((li - 25.0) / li, 0.0);
		//c = blueShift(c, night);
		//c = desaturate(c, night);
		color = mix(color, c, luminanceParams[1]);
	}

	// tone mapping
	if (tonemapSecond[3] > 0.5)
		color = Uncharted2Tonemap(color) / Uncharted2Tonemap(vec3(tonemapSecond[2]));

	// gamma correction
	color = pow(color, vec3(gammaParams[0]));

	outColor = color;
}
