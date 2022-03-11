
$include ../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(location = 0) uniform vec2 uniScale;
layout(binding = 0) uniform sampler2D texColor;
out vec4 outColor;

void main()
{
	vec2 uv = gl_FragCoord.xy * uniScale;
	outColor = textureLod(texColor, uv, 0).rrra;
}
