
const float PI = 3.14159265359;

float sqr(float x)
{
	return x * x;
}

vec2 sqr(vec2 x)
{
	return x * x;
}

vec3 sqr(vec3 x)
{
	return x * x;
}

vec4 sqr(vec4 x)
{
	return x * x;
}

float saturate(float x)
{
	return clamp(x, 0, 1);
}

vec2 saturate(vec2 x)
{
	return clamp(x, vec2(0), vec2(1));
}

vec3 saturate(vec3 x)
{
	return clamp(x, vec3(0), vec3(1));
}

vec4 saturate(vec4 x)
{
	return clamp(x, vec4(0), vec4(1));
}
