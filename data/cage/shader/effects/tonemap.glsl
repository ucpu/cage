
$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

layout(std140, binding = 2) uniform Tonemap
{
	vec4 tonemapFirst; // shoulderStrength, linearStrength, linearAngle, toeStrength
	vec4 tonemapSecond; // toeNumerator, toeDenominator, white, tonemapEnabled
	vec4 gammaParams; // gamma
};

out vec3 outColor;

vec3 uncharted2Tonemap(vec3 x)
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

	// tone mapping
	if (tonemapSecond[3] > 0.5)
	{
		const float exposureBias = 2;
		color = uncharted2Tonemap(color * exposureBias) / uncharted2Tonemap(vec3(tonemapSecond[2]));
	}

	// gamma correction
	color = pow(color, vec3(gammaParams[0]));

	outColor = color;
}
