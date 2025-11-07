
$include vertex.glsl

#ifdef SkeletalAnimation
#error "unintended combination of keywords"
#endif

void main()
{
	propagateInputs();
	computePosition();
}


$include fragment.glsl
$include ../functions/reconstructPosition.glsl

void main()
{
	updateNormal();
	Material material = loadMaterial();
	vec3 prevPos = reconstructPosition(texDepth, uniProjection.vpInv, uniViewport.viewport);
	if (length(varPosition - prevPos) > 0.05)
		discard;
#ifdef CutOut
	if (material.opacity < 0.5)
		discard;
#endif // CutOut
	outColor = lighting(material);
}
