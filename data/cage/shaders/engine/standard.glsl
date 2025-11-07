
$include vertex.glsl

void main()
{
	propagateInputs();
	skeletalAnimation();
	computePosition();
}


$include fragment.glsl

#ifndef CutOut
// layout(early_fragment_tests) in; // not yet supported in tint
#endif

void main()
{
#ifdef DepthOnly

#ifdef CutOut
	if (loadMaterial().opacity < 0.5)
		discard;
#endif // CutOut

#else // DepthOnly

	updateNormal();
	Material material = loadMaterial();
#ifdef CutOut
	if (material.opacity < 0.5)
		discard;
#endif // CutOut
	outColor = lighting(material);

#endif // DepthOnly
}
