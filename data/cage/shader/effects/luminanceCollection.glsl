
$include vertex.glsl

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

out float outLuminance;

float meteringMaskFactor(vec2 uv)
{
	// normalize so that the average mask value over the whole screen is (approximately) one
	// https://www.wolframalpha.com/input/?i=integrate+2.1786+*+%281+-+1.414+*+%28%28x+-+0.5%29%5E2+%2B+%28y+-+0.5%29%5E2%29%5E0.5%29+%3B+x+%3D+0+..+1%3B+y+%3D+0+..+1
	return 2.1786 * (1 - 1.414 * length(uv - 0.5));
}

const float downscale = 4;

void main()
{
	vec2 texelSize = downscale / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = textureLod(texColor, uv, 0).xyz;
	float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
	luminance *= meteringMaskFactor(uv);
	outLuminance = log(max(luminance, 0.0) + 1e-5);
}
