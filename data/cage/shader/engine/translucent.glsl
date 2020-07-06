
$include ../shaderConventions.h

$define translucent 1
$include func/vertexStage.glsl

$define shader fragment
$include func/includes.glsl
$include func/material.glsl
$include func/lighting.glsl

in vec3 varUv;
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varPosition; // world space
flat in int varInstanceId;
out vec4 outColor;

void main()
{
	meshIndex = 0;
	lightIndex = varInstanceId;
	uv = varUv;
	normal = normalize(varNormal);
	tangent = normalize(varTangent);
	bitangent = normalize(varBitangent);
	position = varPosition;
	materialLoad();
	normalToWorld();
	outColor.rgb = lightType();
	if (uniRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] == CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE)
		outColor.rgb += lightEmissiveImpl();
	outColor.a = opacity;
}
