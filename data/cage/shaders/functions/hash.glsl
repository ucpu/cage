
// integer finalizer hash function
uint hash(uint key)
{
	key ^= key >> 16;
	key *= 0x85ebca6b;
	key ^= key >> 13;
	key *= 0xc2b2ae35;
	key ^= key >> 16;
	return key;
}

float hash(float p)
{
	p = fract(p * 0.011);
	p *= p + 7.5;
	p *= p + p;
	return fract(p);
}

float hash(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.13);
	p3 += dot(p3, p3.yzx + 3.333);
	return fract((p3.x + p3.y) * p3.z);
}
