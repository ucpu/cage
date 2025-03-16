
// https://iquilezles.org/articles/biplanar/

struct Biplanar
{
	vec3 normal;
	ivec3 a1; // dominant axis indices
	ivec3 a2; // second axis indices
	ivec3 a3; // least significant axis indices
	vec2 uv1, uv2;
	vec2 dx1, dx2;
	vec2 dy1, dy2;
	vec2 w;
};

Biplanar biplanarPrepare(vec3 p, vec3 n, float k)
{
	Biplanar bip;
	bip.normal = n;
	vec3 dpdx = dFdx(p);
	vec3 dpdy = dFdy(p);
	n = abs(n);
	bip.a1 = (n.x > n.y && n.x > n.z) ? ivec3(0, 1, 2) : (n.y > n.z) ? ivec3(1, 2, 0) : ivec3(2, 0, 1);
	bip.a3 = (n.x < n.y && n.x < n.z) ? ivec3(0, 1, 2) : (n.y < n.z) ? ivec3(1, 2, 0) : ivec3(2, 0, 1);
	bip.a2 = ivec3(3) - bip.a3 - bip.a1;
	bip.uv1 = vec2(p[bip.a1.y], p[bip.a1.z]);
	bip.uv2 = vec2(p[bip.a2.y], p[bip.a2.z]);
	bip.dx1 = vec2(dpdx[bip.a1.y], dpdx[bip.a1.z]);
	bip.dx2 = vec2(dpdx[bip.a2.y], dpdx[bip.a2.z]);
	bip.dy1 = vec2(dpdy[bip.a1.y], dpdy[bip.a1.z]);
	bip.dy2 = vec2(dpdy[bip.a2.y], dpdy[bip.a2.z]);
	bip.w = vec2(n[bip.a1.x], n[bip.a2.x]);
	bip.w = saturate((bip.w - 0.5773) / (1 - 0.5773));
	bip.w = pow(bip.w, vec2(k / 8));
	bip.w /= bip.w[0] + bip.w[1];
	return bip;
}

vec4 biplanarSample(sampler2D sam, Biplanar bip)
{
	vec4 v1 = textureGrad(sam, bip.uv1, bip.dx1, bip.dy1);
	vec4 v2 = textureGrad(sam, bip.uv2, bip.dx2, bip.dy2);
	return v1 * bip.w[0] + v2 * bip.w[1];
}

vec4 biplanarSample(sampler2DArray sam, Biplanar bip, float arrayIndex)
{
	vec4 v1 = textureGrad(sam, vec3(bip.uv1, arrayIndex), bip.dx1, bip.dy1);
	vec4 v2 = textureGrad(sam, vec3(bip.uv2, arrayIndex), bip.dx2, bip.dy2);
	return v1 * bip.w[0] + v2 * bip.w[1];
}

vec3 egacBiplanarNormal(Biplanar bip, vec3 n1, vec3 n2)
{
	// https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
	// whiteout blend
	n1.x *= -1;
	n2.x *= -1;
	n1 = vec3(n1.xy + vec2(bip.normal[bip.a1.z], bip.normal[bip.a1.y]), abs(n1.z) * bip.normal[bip.a1.x]);
	n2 = vec3(n2.xy + vec2(bip.normal[bip.a2.z], bip.normal[bip.a2.y]), abs(n2.z) * bip.normal[bip.a2.x]);
	n1 = vec3(n1[bip.a1.z], n1[bip.a1.y], n1[bip.a1.x]);
	n2 = vec3(n2[bip.a2.z], n2[bip.a2.y], n2[bip.a2.x]);
	return n1 * bip.w[0] + n2 * bip.w[1];
}

vec3 biplanarSampleNormal(sampler2D sam, Biplanar bip)
{
	vec3 n1 = restoreNormalMap(textureGrad(sam, bip.uv1, bip.dx1, bip.dy1));
	vec3 n2 = restoreNormalMap(textureGrad(sam, bip.uv2, bip.dx2, bip.dy2));
	return egacBiplanarNormal(bip, n1, n2);
}

vec3 biplanarSampleNormal(sampler2DArray sam, Biplanar bip, float arrayIndex)
{
	vec3 n1 = restoreNormalMap(textureGrad(sam, vec3(bip.uv1, arrayIndex), bip.dx1, bip.dy1));
	vec3 n2 = restoreNormalMap(textureGrad(sam, vec3(bip.uv2, arrayIndex), bip.dx2, bip.dy2));
	return egacBiplanarNormal(bip, n1, n2);
}
