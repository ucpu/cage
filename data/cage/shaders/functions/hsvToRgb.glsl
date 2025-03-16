
vec3 hsvToRgb(vec3 c)
{
	vec4 K = vec4(1, 2.0 / 3, 1.0 / 3, 3);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6 - K.www);
	return c.z * mix(K.xxx, saturate(p - K.xxx), c.y);
}
