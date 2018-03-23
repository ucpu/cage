
$import shaderConventions.h

$define translucent 1
$include func/vertexStage.glsl

$define shader fragment
$include func/includes.glsl
$include func/material.glsl
$include func/lighting.glsl

in vec2 varUv;
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
	normal = normalize(mat3(uniMeshes[meshIndex].normalMat) * normal);
	outColor.rgb = uniLightType();
	outColor.a = opacity;
}
