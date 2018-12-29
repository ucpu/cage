
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

// https://github.com/McNopper/OpenGL/blob/master/Example42/shader/fxaa.frag.glsl
// modified

layout(binding = 0) uniform sampler2D texColor; 

out vec4 outColor;

const float u_lumaThreshold = 0.5;
const float u_mulReduce = 1.0 / 8.0;
const float u_minReduce = 1.0 / 128.0;
const float u_maxSpan = 8.0;
const vec3 toLuma = vec3(0.299, 0.587, 0.114);

void main(void)
{
	vec2 texelStep = 1.0 / vec2(textureSize(texColor, 0));
	vec2 uv = gl_FragCoord.xy * texelStep;
	vec3 rgbM = texture(texColor, uv).rgb;
	if (false)
	{
		outColor = vec4(rgbM, 1.0);
		return;
	}

	vec3 rgbNW = textureOffset(texColor, uv, ivec2(-1, 1)).rgb;
	vec3 rgbNE = textureOffset(texColor, uv, ivec2(1, 1)).rgb;
	vec3 rgbSW = textureOffset(texColor, uv, ivec2(-1, -1)).rgb;
	vec3 rgbSE = textureOffset(texColor, uv, ivec2(1, -1)).rgb;
	float lumaNW = dot(rgbNW, toLuma);
	float lumaNE = dot(rgbNE, toLuma);
	float lumaSW = dot(rgbSW, toLuma);
	float lumaSE = dot(rgbSE, toLuma);
	float lumaM = dot(rgbM, toLuma);
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	if (lumaMax - lumaMin <= lumaMax * u_lumaThreshold)
	{
		outColor = vec4(rgbM, 1.0);
		return;
	}

	vec2 samplingDirection;
	samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * u_mulReduce, u_minReduce);
	float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);
	samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-u_maxSpan), vec2(u_maxSpan)) * texelStep;
	vec3 rgbSampleNeg = texture(texColor, uv + samplingDirection * (1.0/3.0 - 0.5)).rgb;
	vec3 rgbSamplePos = texture(texColor, uv + samplingDirection * (2.0/3.0 - 0.5)).rgb;
	vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;
	vec3 rgbSampleNegOuter = texture(texColor, uv + samplingDirection * (0.0/3.0 - 0.5)).rgb;
	vec3 rgbSamplePosOuter = texture(texColor, uv + samplingDirection * (3.0/3.0 - 0.5)).rgb;
	vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;
	float lumaFourTab = dot(rgbFourTab, toLuma);
	if (lumaFourTab < lumaMin || lumaFourTab > lumaMax)
		outColor = vec4(rgbTwoTab, 1.0);
	else
		outColor = vec4(rgbFourTab, 1.0);

	if (false)
		outColor.r = 1.0;
}
