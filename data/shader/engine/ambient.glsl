
$include ../shaderConventions.h
$include effects/vertex.glsl

$define shader fragment
$include func/includes.glsl
$include func/lightingImpl.glsl
$include func/reconstructPosition.glsl

layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texGbufferAlbedo;
layout(binding = CAGE_SHADER_TEXTURE_SPECIAL) uniform sampler2D texGbufferSpecial;
layout(binding = CAGE_SHADER_TEXTURE_NORMAL) uniform sampler2D texGbufferNormal;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS) uniform sampler2D texAmbientOcclusion;
layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;

layout(location = CAGE_SHADER_UNI_AMBIENTOCCLUSION) uniform int uniAmbientOcclusion;

out vec3 outColor;

void main()
{
	position = reconstructPosition();
	albedo = texelFetch(texGbufferAlbedo, ivec2(gl_FragCoord.xy), 0).rgb;
	vec4 special = texelFetch(texGbufferSpecial, ivec2(gl_FragCoord.xy), 0);
	roughness = special.r;
	metalness = special.g;
	normal = texelFetch(texGbufferNormal, ivec2(gl_FragCoord.xy), 0).xyz;
	outColor = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).rgb;
	float intensity = 1;
	if (uniAmbientOcclusion > 0)
		intensity = texelFetch(texAmbientOcclusion, ivec2(gl_FragCoord.xy / CAGE_SHADER_SSAO_DOWNSCALE), 0).r;
	if (dot(normal, normal) > 0.5)
		outColor += lightAmbientImpl(intensity);
}
