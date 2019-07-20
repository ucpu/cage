
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(location = 0) uniform vec4 uniBloomParams; // threshold

layout(binding = CAGE_SHADER_TEXTURE_COLOR) uniform sampler2D texColor;

out vec3 outBloom;

void main()
{
	vec3 acc = vec3(0.0);
	int cnt = 0;
	for (int y = 0; y < CAGE_SHADER_BLOOM_DOWNSCALE; y++)
	{
		for (int x = 0; x < CAGE_SHADER_BLOOM_DOWNSCALE; x++)
		{
			vec3 c = texelFetch(texColor, ivec2(gl_FragCoord) * CAGE_SHADER_BLOOM_DOWNSCALE + ivec2(x, y) - CAGE_SHADER_BLOOM_DOWNSCALE / 2, 0).rgb;
			//float l = dot(c, vec3(0.2126, 0.7152, 0.0722)); // todo make the "grayscale factor" configurable
			float l = dot(c, vec3(0.3333));
			if (isnan(l))
				continue; // prevent spreading a NaN to other pixels
			int m = int(l > uniBloomParams[0]);
			acc += float(m) * c;
			cnt += m;
		}
	}
	if (cnt > 0)
		outBloom = acc / float(cnt);
	else
		outBloom = vec3(0.0);
}
