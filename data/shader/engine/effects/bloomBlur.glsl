
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

// https://www.opengl.org/discussion_boards/showthread.php/164908-Gaussian-Blur-on-texture

const vec2 gaussFilter5[5] =
{
	vec2(-2.0, 0.06136),
	vec2(-1.0, 0.24477),
	vec2(+0.0, 0.38774),
	vec2(+1.0, 0.24477),
	vec2(+2.0, 0.06136)
};

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texInput;

out vec4 outOutput;

layout(location = 0) uniform vec2 direction;

void main()
{
	vec2 texelSize = 1.0 / textureSize(texInput, 0).xy;
	vec2 center = gl_FragCoord.xy * texelSize;
	outOutput = vec4(0.0);
	for(int i = 0; i < 5; i++)
	{
		vec2 uv = center + direction * gaussFilter5[i].x * texelSize;
		outOutput += textureLod(texInput,  uv, 0) * gaussFilter5[i].y;
	}
}
