
vec3 restoreNormalMap(vec4 n)
{
	n.xy = n.xy * 2 - 1;
	n.z = sqrt(1 - saturate(dot(n.xy, n.xy)));
	return n.xyz;
}
