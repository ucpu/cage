
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

// https://www.opengl.org/discussion_boards/showthread.php/164908-Gaussian-Blur-on-texture

const vec2 gaussFilter7[7] =
{
	vec2(-3.0, 0.015625),
	vec2(-2.0, 0.09375),
	vec2(-1.0, 0.234375),
	vec2(+0.0, 0.3125),
	vec2(+1.0, 0.234375),
	vec2(+2.0, 0.09375),
	vec2(+3.0, 0.015625)
};

const vec2 gaussFilter5[5] =
{
	vec2(-2.0, 0.06136),
	vec2(-1.0, 0.24477),
	vec2(+0.0, 0.38774),
	vec2(+1.0, 0.24477),
	vec2(+2.0, 0.06136)
};

layout(binding = 0) uniform sampler2D texInput;

out vec4 outOutput;

layout(location = 0) uniform vec2 direction;

// todo do not blur over depth or normal discontinuities

void main()
{
	vec2 texelSize = 1.0 / textureSize(texInput, 0).xy;
	vec2 center = gl_FragCoord.xy * texelSize;
	outOutput = vec4(0.0);
	for(int i = 0; i < 7; i++)
	{
		vec2 uv = center + direction * gaussFilter7[i].x * texelSize;
		outOutput += textureLod(texInput,  uv, 0) * gaussFilter7[i].y;
	}
}
