
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

$include ../functions/reconstructPosition.glsl

layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

#ifndef AlphaClip
layout(early_fragment_tests) in;
#endif

void main()
{
	updateNormal();
	Material material = loadMaterial();
	vec3 prevPos = reconstructPosition(texDepth, uniViewport.vpInv, uniViewport.viewport);
	if (length(varPosition - prevPos) > 0.05)
		discard;
#ifdef AlphaClip
	if (material.opacity < 0.5)
		discard;
#endif // AlphaClip
	outColor = lighting(material);
}
