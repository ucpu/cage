
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	varInstanceId = gl_InstanceID;
	varUv = inUv;
	vec3 p3 = mat3(uniViewport.vMat) * mat3(uniMeshes[varInstanceId].mMat) * vec3(inPosition);
	gl_Position = uniViewport.pMat * vec4(p3, 1);
}

$include fragment.glsl

layout(early_fragment_tests) in;

out vec4 outColor;

void main()
{
	Material material = loadMaterial();
	outColor.rgb = material.albedo;
	outColor.a = 1;
}
