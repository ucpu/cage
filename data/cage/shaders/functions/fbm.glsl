
$include noise.glsl

const int fbmOctaves = 5;

float fbm(float x)
{
	float v = 0;
	float a = 0.5;
	const float shift = float(100);
	for (int i = 0; i < fbmOctaves; ++i)
	{
		v += a * noise(x);
		x = x * 2 + shift;
		a *= 0.5;
	}
	return v;
}


float fbm(vec2 x)
{
	float v = 0;
	float a = 0.5;
	const vec2 shift = vec2(100);
    const mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
	for (int i = 0; i < fbmOctaves; ++i)
	{
		v += a * noise(x);
		x = rot * x * 2 + shift;
		a *= 0.5;
	}
	return v;
}


float fbm(vec3 x)
{
	float v = 0;
	float a = 0.5;
	const vec3 shift = vec3(100);
	for (int i = 0; i < fbmOctaves; ++i)
	{
		v += a * noise(x);
		x = x * 2 + shift;
		a *= 0.5;
	}
	return v;
}
