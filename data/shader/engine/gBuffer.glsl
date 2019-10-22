
$include ../shaderConventions.h

$include func/vertexStage.glsl

$define shader fragment
$include func/includes.glsl
$include func/material.glsl
$include func/lightingImpl.glsl

in vec3 varUv;
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varPosition; // world space
in vec4 varPosition4;
in vec4 varPosition4Prev;
flat in int varInstanceId;
layout(location = CAGE_SHADER_ATTRIB_OUT_ALBEDO) out vec3 outAlbedo;
layout(location = CAGE_SHADER_ATTRIB_OUT_SPECIAL) out vec2 outSpecial;
layout(location = CAGE_SHADER_ATTRIB_OUT_NORMAL) out vec3 outNormal;
layout(location = CAGE_SHADER_ATTRIB_OUT_COLOR) out vec3 outColor;
layout(location = CAGE_SHADER_ATTRIB_OUT_VELOCITY) out vec2 outVelocity;

void main()
{
	meshIndex = varInstanceId;
	uv = varUv;
	normal = normalize(varNormal);
	tangent = normalize(varTangent);
	bitangent = normalize(varBitangent);
	materialLoad();
	if (opacity < 0.5)
		discard;
	normalToWorld();
	outColor = lightAmbientImpl();
	outAlbedo = albedo;
	outSpecial = vec2(roughness, metalness);
	outNormal = normal;
	outVelocity = (varPosition4.xy / varPosition4.w - varPosition4Prev.xy / varPosition4Prev.w);
}
