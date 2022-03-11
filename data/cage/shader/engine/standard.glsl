
$include ../shaderConventions.h

$include vertexStage.glsl

$define shader fragment

in vec3 varUv;
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varPosition; // world space
flat in int varInstanceId;
out vec4 outColor;

$include fragmentStage.glsl

void main()
{
	uv = varUv;
	normal = normalize(varNormal);
	tangent = normalize(varTangent);
	bitangent = normalize(varBitangent);
	position = varPosition;
	materialLoad();
	normalLoad();
	normalToWorld();
	outColor.rgb = lightType();
	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] == CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE)
		outColor.rgb += lightEmissiveImpl();
	outColor.a = opacity;
}
