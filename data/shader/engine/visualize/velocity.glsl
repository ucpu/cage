
$include ../../shaderConventions.h
$include vertex.glsl

$define shader fragment

layout(location = 0) uniform vec2 uniScale;
layout(binding = 0) uniform sampler2D texVelocity;
out vec3 outColor;

void main()
{
	vec2 uv = gl_FragCoord.xy * uniScale;
	vec2 v = textureLod(texVelocity, uv, 0).xy;
	float s = max(length(v) - 1e-5, 0.0);
	outColor = vec3(abs(v) * pow(s * 5.0, 0.2) / (s + 1e-5), 0.0);
}
