
$include ../../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_EFFECTS) uniform sampler2D texLuminanceOld;
layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texLuminanceNew;
layout(location = 0) uniform vec2 uniAdaptationSpeed; // darker, lighter

out float outLuminance;

void main()
{
	float old = textureLod(texLuminanceOld, vec2(0.0), 100).x;
	float new = textureLod(texLuminanceNew, vec2(0.0), 100).x;
	outLuminance = old + (new - old) * (1.0 - exp(-uniAdaptationSpeed[new > old ? 1 : 0]));
}
