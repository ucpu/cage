
$import shaderConventions.h

$define shader vertex
$include func/includes.glsl
$include func/lighting.glsl

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
flat out int varInstanceId;

void main()
{
	vec4 pos = vec4(inPosition, 1.0);
	gl_Position = uniLights[gl_InstanceID].mvpMat * pos;
	varInstanceId = gl_InstanceID;
}



$define shader fragment
$include func/includes.glsl
$include func/lighting.glsl
$include func/reconstructPosition.glsl

layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texGbufferAlbedo;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL) uniform sampler2D texGbufferSpecial;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texGbufferNormal;

flat in int varInstanceId;
out vec3 outColor;

void main()
{
	lightIndex = varInstanceId;
	position = reconstructPosition();
	albedo = texelFetch(texGbufferAlbedo, ivec2(gl_FragCoord.xy), 0).rgb;
	vec4 special = texelFetch(texGbufferSpecial, ivec2(gl_FragCoord.xy), 0);
	roughness = special.r;
	metalness = special.g;
	emissive = 0;
	smoothNormal = normal = texelFetch(texGbufferNormal, ivec2(gl_FragCoord.xy), 0).xyz;
	outColor = uniLightType();
}
