uint hash(uint key)
{ // integer finalizer hash function
	key ^= key >> 16;
	key *= 0x85ebca6b;
	key ^= key >> 13;
	key *= 0xc2b2ae35;
	key ^= key >> 16;
	return key;
}

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float intToFloat(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32
    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0
    float f = uintBitsToFloat(m);          // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float randomFunc(float x)
{
	return intToFloat(hash(floatBitsToUint(x)));
}
float randomFunc(vec2 v)
{
	return intToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y))));
}
float randomFunc(vec3 v)
{
	return intToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y)) ^ hash(floatBitsToUint(v.z))));
}
float randomFunc(vec4 v)
{
	return intToFloat(hash(floatBitsToUint(v.x) ^ hash(floatBitsToUint(v.y)) ^ hash(floatBitsToUint(v.z)) ^ hash(floatBitsToUint(v.w))));
}
