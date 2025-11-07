
$include vertex.glsl

$define shader fragment

// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

const float offsets[3] = { 0.0, 1.3846153846, 3.2307692308 };
const float weights[3] = { 0.2270270270, 0.3162162162, 0.0702702703 };

#ifdef Vertical
	const vec2 direction = vec2(0, 1);
#else
	const vec2 direction = vec2(1, 0);
#endif

layout(set = 2, binding = 0) uniform sampler2D texInput;

layout(location = 0) out vec4 outOutput;

void main()
{
	vec2 texelSize = 1 / vec2(textureSize(texInput, 0));
	vec2 center = gl_FragCoord.xy;
	vec4 val = textureLod(texInput, center * texelSize, 0) * weights[0];
	for(int i = 1; i < 3; i++)
	{
		val += textureLod(texInput, (center - direction * offsets[i]) * texelSize, 0) * weights[i];
		val += textureLod(texInput, (center + direction * offsets[i]) * texelSize, 0) * weights[i];
	}
	outOutput = val;
}
