
// anim: time (seconds), speed, offset (normalized), unused
// mat: animation duration, animation loops, unused, unused
vec4 sampleTextureAnimation(sampler2DArray tex, vec2 uv, vec4 anim, vec4 mat)
{
	int frames = textureSize(tex, 0)[2];
	if (frames <= 1)
		return texture(tex, vec3(uv, 0));
	float sampl = (anim[0] * anim[1] / mat[0] + anim[2]) * float(frames);
	if (mat[1] > 0.5)
		sampl += (-int(sampl) / frames + 1) * frames;
	else
		sampl = clamp(sampl, 0, float(frames) - 1);
	int i = int(sampl) % frames;
	float f = sampl - float(int(sampl));
	vec4 a = texture(tex, vec3(uv, float(i)));
	vec4 b = texture(tex, vec3(uv, float((i + 1) % frames)));
	return mix(a, b, f);
}
