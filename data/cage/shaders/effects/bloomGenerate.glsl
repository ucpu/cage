
$include vertex.glsl

$define shader fragment

layout(std430, set = 2, binding = 0) readonly buffer Params
{
	vec4 uniBloomParams; // threshold, blur passes
};

layout(set = 2, binding = 1) uniform sampler2D texColor;

layout(location = 0) out vec4 outBloom;

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
			if (!(l >= 0.0 || l < 0.0)) // if (isnan(l))
				continue; // prevent spreading a NaN to other pixels
			int m = int(l > uniBloomParams[0]);
			acc += float(m) * c;
			cnt += m;
		}
	}
	if (cnt > 0)
		outBloom = vec4(acc / float(cnt), 1);
	else
		outBloom = vec4(0, 0, 0, 1);
}
