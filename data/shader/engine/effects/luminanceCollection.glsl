
$include ../../shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;

out float outLuminance;

void main()
{
	vec2 texelSize = float(CAGE_SHADER_LUMINANCE_DOWNSCALE) / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = textureLod(texColor, uv, 0).xyz;
	float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
	outLuminance = log(max(luminance, 0.0) + 1e-6);
}
