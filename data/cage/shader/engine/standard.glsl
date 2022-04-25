
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

#ifndef AlphaClip
layout(early_fragment_tests) in;
#endif

void main()
{
#ifdef DepthOnly

#ifdef AlphaClip
	if (loadMaterial().opacity < 0.5)
		discard;
#endif // AlphaClip

#else // DepthOnly

	updateNormal();
	Material material = loadMaterial();
#ifdef AlphaClip
	if (material.opacity < 0.5)
		discard;
#endif // AlphaClip
	outColor = lighting(material);

#endif // DepthOnly
}
