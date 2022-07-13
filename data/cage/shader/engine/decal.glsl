
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

$include ../functions/reconstructPosition.glsl

layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;

void main()
{
	vec3 prevPos = reconstructPosition(texDepth, uniViewport.vpInv, uniViewport.viewport);
	if (length(varPosition - prevPos) > 0.05)
		discard;
	updateNormal();
	Material material = loadMaterial();
#ifdef AlphaClip
	if (material.opacity < 0.5)
		discard;
#endif // AlphaClip
	outColor = lighting(material);
}
