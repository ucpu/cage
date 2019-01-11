
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_LUMINANCE) uniform sampler2D texLuminance;

layout(std140, binding = CAGE_SHADER_UNIBLOCK_FINALSCREEN) uniform FinalScreen
{
	vec4 tonemapFirst; // shoulderStrength, linearStrength, linearAngle, toeStrength
	vec4 tonemapSecond; // toeNumerator, toeDenominator, white, tonemapEnabled
	vec4 luminanceParams; // exposureKey, adaptationStrength
	vec4 gammaParams; // gamma
};

out vec3 outColor;

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

	// exposure control
	if (luminanceParams[1] > 0.0)
	{
		float avgLuminance = texelFetch(texLuminance, ivec2(0), 0).x;
		avgLuminance = exp(avgLuminance);
		//avgLuminance = mix(1.0, avgLuminance, luminanceParams[1]);
		color *= luminanceParams[0] / avgLuminance;
	}

	// tone mapping
	if (tonemapSecond[3] > 0.5)
		color = Uncharted2Tonemap(color) / Uncharted2Tonemap(vec3(tonemapSecond[2]));

	// gamma correction
	color = pow(color, vec3(gammaParams[0]));

	outColor = color;
}
