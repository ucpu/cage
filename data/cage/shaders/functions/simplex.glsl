// https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83

float egacPermute(float x)
{
	return floor(mod(((x * 34) + 1) * x, 289));
}

vec4 egacPermute(vec4 x)
{
	return mod(((x * 34) + 1) * x, 289);
}

float egacTaylorInvSqrt(float r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 egacTaylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 egacGrad4(float j, vec4 ip)
{
	const vec4 ones = vec4(1, 1, 1, -1);
	vec4 p, s;
	p.xyz = floor(fract(vec3(j) * ip.xyz) * 7) * ip.z - 1;
	p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
	s = vec4(lessThan(p, vec4(0)));
	p.xyz = p.xyz + (s.xyz * 2 - 1) * s.www;
	return p;
}

float simplex(vec4 v)
{
	const vec2 C = vec2(0.138196601125010504, 0.309016994374947451);
	vec4 i = floor(v + dot(v, C.yyyy));
	vec4 x0 = v - i + dot(i, C.xxxx);
	vec4 i0;
	vec3 isX = step(x0.yzw, x0.xxx);
	vec3 isYZ = step(x0.zww, x0.yyz);
	i0.x = isX.x + isX.y + isX.z;
	i0.yzw = 1 - isX;
	i0.y += isYZ.x + isYZ.y;
	i0.zw += 1 - isYZ.xy;
	i0.z += isYZ.z;
	i0.w += 1 - isYZ.z;
	vec4 i3 = saturate(i0);
	vec4 i2 = saturate(i0 - 1);
	vec4 i1 = saturate(i0 - 2);
	vec4 x1 = x0 - i1 + 1 * C.xxxx;
	vec4 x2 = x0 - i2 + 2 * C.xxxx;
	vec4 x3 = x0 - i3 + 3 * C.xxxx;
	vec4 x4 = x0 - 1 + 4 * C.xxxx;
	i = mod(i, 289);
	float j0 = egacPermute(egacPermute(egacPermute(egacPermute(i.w) + i.z) + i.y) + i.x);
	vec4 j1 = egacPermute(egacPermute(egacPermute(egacPermute(i.w + vec4(i1.w, i2.w, i3.w, 1)) + i.z + vec4(i1.z, i2.z, i3.z, 1)) + i.y + vec4(i1.y, i2.y, i3.y, 1)) + i.x + vec4(i1.x, i2.x, i3.x, 1));
	const vec4 ip = vec4(1 / 294.0, 1 / 49.0, 1 / 7.0, 0);
	vec4 p0 = egacGrad4(j0, ip);
	vec4 p1 = egacGrad4(j1.x, ip);
	vec4 p2 = egacGrad4(j1.y, ip);
	vec4 p3 = egacGrad4(j1.z, ip);
	vec4 p4 = egacGrad4(j1.w, ip);
	vec4 norm = egacTaylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;
	p4 *= egacTaylorInvSqrt(dot(p4, p4));
	vec3 m0 = max(0.6 - vec3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0);
	vec2 m1 = max(0.6 - vec2(dot(x3, x3), dot(x4, x4)), 0);
	m0 = m0 * m0;
	m1 = m1 * m1;
	return 49 * (dot(m0 * m0, vec3(dot(p0, x0), dot(p1, x1), dot(p2, x2))) + dot(m1 * m1, vec2(dot(p3, x3), dot(p4, x4))));
}
