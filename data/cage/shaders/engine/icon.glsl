
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	varInstanceId = gl_InstanceID;
	varPosition = inPosition;
	varUv = inUv;
	gl_Position = uniProjection.vpMat * transpose(mat4(uniMeshes[varInstanceId].modelMat)) * vec4(varPosition, 1);
}

$include fragment.glsl

#ifndef CutOut
layout(early_fragment_tests) in;
#endif

void main()
{
	vec4 albedo = texture(texMaterialAlbedo2d, varUv.xy);
#ifdef CutOut
	if (albedo.w < 0.5)
		discard;
#endif // CutOut
	outColor = albedo;
}
