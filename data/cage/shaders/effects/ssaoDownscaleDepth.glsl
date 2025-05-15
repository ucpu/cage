
$include ../shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec3 inUv;
out vec2 varUv;

void main()
{
	gl_Position = vec4(inPosition.xy * 2 - 1, inPosition.z, 1);
	varUv = inUv.xy;
	varUv.y = 1 - varUv.y;
}

$define shader fragment

layout(binding = 0) uniform sampler2D texDepth;
in vec2 varUv;
out float outDepth;

void main()
{
	outDepth = textureLod(texDepth, varUv, 0).x;
}
