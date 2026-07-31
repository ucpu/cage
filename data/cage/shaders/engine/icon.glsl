
$include vertex.glsl

#ifdef SkeletalAnimation
#error "unintended combination of keywords"
#endif

void main()
{
	propagateInputs();
	gl_Position = uniProjection.vpMat * transpose(mat4(uniMeshes[varInstanceId].modelMat)) * vec4(varPosition, 1);
}


$include fragment.glsl

#ifndef CutOut
layout(early_fragment_tests) in;
#endif

void main()
{
	vec4 albedo = loadAlbedo();

#ifdef CutOut
	if (albedo.w < 0.5)
		discard;
#endif // CutOut

	outColor = albedo;
}
