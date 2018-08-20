
$import shaderConventions.h

$include func/vertexStage.glsl

$define shader fragment
$include func/includes.glsl
$include func/material.glsl
$include func/lightingImpl.glsl

in vec2 varUv;
in vec3 varNormal; // object space
in vec3 varTangent; // object space
in vec3 varBitangent; // object space
in vec3 varPosition; // world space
flat in int varInstanceId;
layout(location = CAGE_SHADER_ATTRIB_OUT_ALBEDO) out vec4 outAlbedo;
layout(location = CAGE_SHADER_ATTRIB_OUT_SPECIAL) out vec4 outSpecial;
layout(location = CAGE_SHADER_ATTRIB_OUT_NORMAL) out vec4 outNormal;
layout(location = CAGE_SHADER_ATTRIB_OUT_COLOR) out vec4 outColor;

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
	{
		mat4 nm = uniMeshes[meshIndex].normalMat;
		if (nm[3][3] > 0.5) // is ligting enabled
			normal = normalize(mat3(nm) * normal);
		else
			normal = vec3(0.0);
	}
	outColor.rgb = lightAmbientImpl();
	outAlbedo.rgb = albedo;
	outSpecial.rg = vec2(roughness, metalness);
	outNormal.xyz = normal;
}
