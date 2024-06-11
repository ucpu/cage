
$include hash.glsl

float egacIntToFloat(uint m)
{
	const uint ieeeMantissa = 0x007FFFFFu;
	const uint ieeeOne = 0x3F800000u;
	m &= ieeeMantissa;
	m |= ieeeOne;
	float f = uintBitsToFloat(m);
	return f - 1;
}

float randomFunc(float x)
{
	return egacIntToFloat(hash(floatBitsToUint(x)));
}
float randomFunc(vec2 v)
{
	return egacIntToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y))));
}
float randomFunc(vec3 v)
{
	return egacIntToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y)) ^ hash(floatBitsToUint(v.z))));
}
float randomFunc(vec4 v)
{
	return egacIntToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y)) ^ hash(floatBitsToUint(v.z)) ^ hash(floatBitsToUint(v.w))));
}
