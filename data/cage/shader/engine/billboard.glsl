
$include ../shaderConventions.h

$include vertex.glsl

mat4 removeRotation(mat4 m)
{
	m[0].xyz = vec3(1, 0, 0);
	m[1].xyz = vec3(0, 1, 0);
	m[2].xyz = vec3(0, 0, 1);
	return m;
}

void main()
{
	varInstanceId = gl_InstanceID;
	varPosition = inPosition;
	varNormal = inNormal;
	varUv = inUv;
	skeletalAnimation();

	mat4 m = mat4(transpose(uniMeshes[varInstanceId].mMat));
	mat4 mvp = uniViewport.pMat * removeRotation(uniViewport.vMat * m);
	m = removeRotation(m);
	gl_Position = mvp * vec4(varPosition, 1);
	varPosition = vec3(m * vec4(varPosition, 1));
}

$include fragment.glsl

layout(early_fragment_tests) in;

void main()
{
	updateNormal();
	normal = inverse(mat3(uniMeshes[varInstanceId].normalMat)) * normal; // revert normal rotation
	Material material = loadMaterial();
	outColor = lighting(material);
}
