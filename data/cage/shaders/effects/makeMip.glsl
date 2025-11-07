
$include vertex.glsl

$define shader fragment

layout(set = 2, binding = 0) uniform sampler2D texColor;

layout(location = 0) out vec4 outColor;

void main()
{
	vec2 uv = 2 * vec2(gl_FragCoord) / vec2(textureSize(texColor, 0));
	outColor = textureLod(texColor, uv, 0);
}
