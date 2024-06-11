
$include hash.glsl

float noise(float x)
{
	float i = floor(x);
	float f = fract(x);
	float u = f * f * (3 - 2 * f);
	return mix(hash(i), hash(i + 1), u);
}

float noise(vec2 x)
{
	vec2 i = floor(x);
	vec2 f = fract(x);
	float a = hash(i);
	float b = hash(i + vec2(1, 0));
	float c = hash(i + vec2(0, 1));
	float d = hash(i + vec2(1, 1));
	vec2 u = f * f * (3 - 2 * f);
	return mix(a, b, u.x) + (c - a) * u.y * (1 - u.x) + (d - b) * u.x * u.y;
}

float noise(vec3 x)
{
	const vec3 step = vec3(110, 241, 171);
	vec3 i = floor(x);
	vec3 f = fract(x);
	float n = dot(i, step);
	vec3 u = f * f * (3 - 2 * f);
	return mix(mix(mix(hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),
				   mix(hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),
			   mix(mix(hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),
				   mix(hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);
}
