
$import shaderConventions.h

$define shader vertex

layout(location = 0) uniform vec4 pos;
layout(location = 1) uniform vec4 uv;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
layout(location = CAGE_SHADER_ATTRIB_IN_UV) in vec2 inUv;
out vec2 varUv;

void main()
{
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
	gl_Position.xy = pos.xy + (pos.zw - pos.xy) * inUv;
	varUv = uv.xy + (uv.zw - uv.xy) * inUv;
}

$define shader fragment

$if inputSpec ^ 0 = A
	layout(location = 2) uniform vec4  aniTexFrames;
	layout(binding = 0) uniform sampler2DArray texImg;
	$include ../engine/func/sampleTextureAnimation.glsl
$else
	layout(binding = 0) uniform sampler2D texImg;
$end
in vec2 varUv;
out vec4 outColor;

void main()
{
$if inputSpec ^ 0 = A
	outColor = sampleTextureAnimation(texImg, varUv, aniTexFrames);
$else
	outColor = texture(texImg, varUv);
$end
}
