
$include ../shaderConventions.h

$include func/vertexStage.glsl

$define shader fragment
$include func/includes.glsl
$include func/material.glsl

in vec3 varUv;
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varPosition; // world space
flat in int varInstanceId;

void main()
{
	meshIndex = varInstanceId;
	uv = varUv;
	normal = normalize(varNormal);
	tangent = normalize(varTangent);
	bitangent = normalize(varBitangent);
	position = varPosition;
	materialLoad();
	if (opacity < 0.5)
		discard;
}
