
$include ../shaderConventions.h
$include vertex.glsl

$define shader fragment

$include dofParams.glsl

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texDepth;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS + 0) uniform sampler2D texDofNear;
layout(binding = CAGE_SHADER_TEXTURE_EFFECTS + 1) uniform sampler2D texDofFar;

out vec3 outColor;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = texelFetch(texColor, ivec2(gl_FragCoord), 0).rgb;
	float depth = texelFetch(texDepth, ivec2(gl_FragCoord), 0).x;
	vec3 colorNear = textureLod(texDofNear, uv, 0).rgb;
	vec3 colorFar = textureLod(texDofFar, uv, 0).rgb;
	float near;
	float far;
	dofContribution(uv, depth, near, far);
	outColor = color * (1 - near - far) + colorNear * near + colorFar * far;
}
