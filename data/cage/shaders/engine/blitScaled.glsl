
layout(location = 0) varying vec2 varUv;


$define shader vertex

layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec3 inUv;

void main()
{
	gl_Position = vec4(inPosition.xy * 2 - 1, inPosition.z, 1);
	varUv = inUv.xy;
	varUv.y = 1 - varUv.y;
}


$define shader fragment

layout(set = 2, binding = 0) uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = textureLod(texColor, varUv, 0);
}
