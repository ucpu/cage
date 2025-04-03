
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

const float offsets[3] = { 0.0, 1.3846153846, 3.2307692308 };
const float weights[3] = { 0.2270270270, 0.3162162162, 0.0702702703 };

layout(binding = 0) uniform sampler2D texInput;

out vec4 outOutput;

layout(location = 0) uniform vec2 uniDirection;
layout(location = 1) uniform int uniLodLevel;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texInput, uniLodLevel).xy;
	vec2 center = gl_FragCoord.xy;
	vec4 val = textureLod(texInput, center * texelSize, uniLodLevel) * weights[0];
	for(int i = 1; i < 3; i++)
	{
		val += textureLod(texInput, (center - uniDirection * offsets[i]) * texelSize, uniLodLevel) * weights[i];
		val += textureLod(texInput, (center + uniDirection * offsets[i]) * texelSize, uniLodLevel) * weights[i];
	}
	outOutput = val;
}
