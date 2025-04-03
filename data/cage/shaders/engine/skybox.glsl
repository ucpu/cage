
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	varInstanceId = gl_InstanceID;
	varUv = inUv;
	vec3 p3 = mat3(uniProjection.viewMat) * transpose(mat3(uniMeshes[varInstanceId].modelMat)) * vec3(inPosition);
	gl_Position = uniProjection.projMat * vec4(p3, 1);
}

$include fragment.glsl

layout(early_fragment_tests) in;

void main()
{
	Material material = loadMaterial();
	outColor.rgb = material.albedo;
	outColor.a = 1;
}
