
vec4 sampleTextureAnimation(sampler2DArray tex, vec2 uv, vec4 frames)
{
	vec4 a = texture(tex, vec3(uv, frames.x));
	vec4 b = texture(tex, vec3(uv, frames.y));
	return mix(a, b, frames.z);
}
