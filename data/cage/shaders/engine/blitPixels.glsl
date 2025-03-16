
$include ../shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2 - 1, inPosition.z, 1);
}

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;
out vec4 outColor;

void main()
{
	outColor = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0);
}
