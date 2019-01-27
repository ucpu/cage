
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

$include ssaoParams.glsl

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_ALBEDO) uniform sampler2D texAlbedo;
layout(binding = CAGE_SHADER_TEXTURE_AMBIENTOCCLUSION) uniform sampler2D texAo;

out vec3 outColor;

void main()
{
	float ao = textureLod(texAo, gl_FragCoord.xy / textureSize(texColor, 0).xy, 0).x;
	vec3 albedo = texelFetch(texAlbedo, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0).xyz;
	ao = pow(ao * params[0] + params[1], params[2]);
	outColor = color - albedo * vec3(ambientLight) * ao;
}
