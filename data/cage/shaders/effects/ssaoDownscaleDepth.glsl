
$include vertex.glsl

$define shader fragment

layout(set = 2, binding = 0) uniform sampler2D texDepth;

layout(location = 0) out float outDepth;

const int downscale = 3;

void main()
{
	vec2 uv = downscale * vec2(gl_FragCoord) / vec2(textureSize(texDepth, 0));
	outDepth = textureLod(texDepth, uv, 0).x;
}
