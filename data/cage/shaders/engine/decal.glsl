
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
	vec3 prevPos = reconstructPosition(texDepth, uniProjection.vpInv, uniViewport.viewport);
	if (length(varPosition - prevPos) > 0.05)
		discard;
	updateNormal();
	Material material = loadMaterial();
#ifdef CutOut
	if (material.opacity < 0.5)
		discard;
#endif // CutOut
	outColor = lighting(material);
}
