
$include vertex.glsl

$define shader fragment

layout(location = 0) uniform vec4 uniBloomParams; // threshold

layout(binding = 0) uniform sampler2D texColor;

out vec3 outBloom;

const int downscale = 3;

void main()
{
	vec3 acc = vec3(0.0);
	int cnt = 0;
	for (int y = 0; y < downscale; y++)
	{
		for (int x = 0; x < downscale; x++)
		{
			vec3 c = texelFetch(texColor, ivec2(gl_FragCoord) * downscale + ivec2(x, y) - downscale / 2, 0).rgb;
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
