
$include ../../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(location = 0) uniform vec2 uniScale;
layout(binding = 0) uniform sampler2D texDepth;
out vec3 outColor;

void main()
{
	vec2 uv = gl_FragCoord.xy * uniScale;
	float v = textureLod(texDepth, uv, 0).r;
	if (v < 1e-7)
		outColor = vec3(1.0, 0.0, 0.0);
	else if (v > 1 - 1e-7)
		outColor = vec3(0.0, 1.0, 0.0);
	else
		outColor = vec3(v);
}
