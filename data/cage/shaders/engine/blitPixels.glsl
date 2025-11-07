
$define shader vertex

layout(location = 0) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2 - 1, inPosition.z, 1);
}


$define shader fragment

layout(set = 2, binding = 0) uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texelFetch(texColor, ivec2(gl_FragCoord.xy), 0);
}
