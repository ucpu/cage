
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

	mat4 m = transpose(mat4(uniMeshes[varInstanceId].modelMat));
	mat4 mvp = uniProjection.projMat * removeRotation(uniProjection.viewMat * m);
	m = removeRotation(m);
	gl_Position = mvp * vec4(varPosition, 1);
	varPosition = vec3(m * vec4(varPosition, 1));
}

$include fragment.glsl

layout(early_fragment_tests) in;

void main()
{
	updateNormal();
	mat3 nm = transpose(mat3(uniMeshes[varInstanceId].modelMat));
	normal = inverse(nm) * normal; // revert normal rotation
	Material material = loadMaterial();
	outColor = lighting(material);
}
