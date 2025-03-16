
vec3 egacFresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

float egacDistributionGGX(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1) + 1;
	denom = PI * denom * denom;
	return a2 / denom;
}

float egacGeometrySchlickGGX(float roughness, float NoV)
{
	float r = roughness + 1;
	float k = r * r / 8;
	float denom = NoV * (1 - k) + k;
	return NoV / denom;
}

float egacGeometrySmith(float roughness, float NoL, float NoV)
{
	float ggx1 = egacGeometrySchlickGGX(roughness, NoL);
	float ggx2 = egacGeometrySchlickGGX(roughness, NoV);
	return ggx1 * ggx2;
}

vec3 brdf(vec3 N, vec3 L, vec3 V, vec3 albedo, float roughness, float metalness)
{
	vec3 H = normalize(L + V);
	float NoL = max(dot(N, L), 0);
	float NoV = max(dot(N, V), 0);
	float NoH = max(dot(N, H), 0); 
	float VoH = max(dot(V, H), 0);

	// https://learnopengl.com/PBR/Lighting
	// https://github.com/pumexx/pumex/blob/86fda7fa351d00bd5918ad90899ce2d6bb8b1dfe/examples/pumexdeferred/shaders/deferred_composite.frag
	vec3 F0 = mix(vec3(0.04), albedo, metalness);
	vec3 F = egacFresnelSchlick(F0, VoH); // VoH or NoV
	vec3 kD = (vec3(1) - F) * (1 - metalness);

	float NDF = egacDistributionGGX(roughness, NoH);
	float G = egacGeometrySmith(roughness, NoL, NoV);

	vec3 nominator = NDF * G * F;
	float denominator = 4 * NoV * NoL + 0.001;
	return min(kD * albedo / PI + nominator / denominator, 1) * NoL;
}
