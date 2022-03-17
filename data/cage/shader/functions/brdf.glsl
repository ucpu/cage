
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(float roughness, float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1.0) + 1.0;
	denom = 3.14159265359 * denom * denom;
	return a2 / denom;
}

float geometrySchlickGGX(float roughness, float NoV)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}

float geometrySmith(float roughness, float NoL, float NoV)
{
	float ggx1 = geometrySchlickGGX(roughness, NoL);
	float ggx2 = geometrySchlickGGX(roughness, NoV);
	return ggx1 * ggx2;
}

vec3 brdf(vec3 N, vec3 L, vec3 V, vec3 albedo, float roughness, float metalness)
{
	vec3 H = normalize(L + V);
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0); 
	float VoH = max(dot(V, H), 0.0);

	// https://learnopengl.com/PBR/Lighting
	// https://github.com/pumexx/pumex/blob/86fda7fa351d00bd5918ad90899ce2d6bb8b1dfe/examples/pumexdeferred/shaders/deferred_composite.frag
	vec3 F0 = mix(vec3(0.04), albedo, metalness);
	vec3 F = fresnelSchlick(F0, VoH); // VoH or NoV
	vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);

	float NDF = distributionGGX(roughness, NoH);
	float G = geometrySmith(roughness, NoL, NoV);

	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * NoV * NoL + 0.001;
	return min(kD * albedo / 3.14159265359 + nominator / denominator, 1.0) * NoL;
}
