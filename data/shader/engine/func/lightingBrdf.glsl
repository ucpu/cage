
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(float NoH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH2 = NoH * NoH;
	float denom = NoH2 * (a2 - 1.0) + 1.0;
	denom = 3.14159265359 * denom * denom;
	return a2 / denom;
}
float GeometrySchlickGGX(float NoV)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}
float GeometrySmith(float NoL, float NoV)
{
	float ggx1 = GeometrySchlickGGX(NoL);
	float ggx2 = GeometrySchlickGGX(NoV);
	return ggx1 * ggx2;
}

vec3 lightingBrdfPbr(vec3 light, vec3 L, vec3 V)
{
	vec3 N = normal;
	if (dot(N, N) < 0.5)
		return vec3(0.0); // lighting is disabled

	vec3 H = normalize(L + V);
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0); 
	float VoH = max(dot(V, H), 0.0);

	// https://learnopengl.com/PBR/Lighting
	// https://github.com/pumexx/pumex/blob/86fda7fa351d00bd5918ad90899ce2d6bb8b1dfe/examples/pumexdeferred/shaders/deferred_composite.frag
	vec3 F0 = mix(vec3(0.04), albedo, metalness);
	vec3 F = fresnelSchlick(VoH, F0); // VoH or NoV
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;

	float NDF = DistributionGGX(NoH);
	float G = GeometrySmith(NoL, NoV);

	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * NoV * NoL + 0.001;
	return min(kD * albedo / 3.14159265359 + nominator / denominator, 100.0) * light * NoL;
}

vec3 lightingBrdfPhong(vec3 light, vec3 L, vec3 V)
{
	vec3 N = normal;
	if (dot(N, N) < 0.5)
		return vec3(0.0); // lighting is disabled

	//vec3 H = normalize(L + V);
	vec3 R = reflect(-L, N);
	
	float shininess = 2.0 / (roughness * roughness) - 2.0;
	float d = max(dot(L, N), 0.0);
	float s = pow(max(dot(R, V), 0.0), shininess);

	return albedo * d + vec3(s);
}

vec3 lightingBrdf(vec3 light, vec3 L, vec3 V)
{
	switch (uniRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTBRDF])
	{
	case CAGE_SHADER_ROUTINEPROC_LIGHTBRDFPBR: return lightingBrdfPbr(light, L, V);
	case CAGE_SHADER_ROUTINEPROC_LIGHTBRDFPHONG: return lightingBrdfPhong(light, L, V);
	default: return vec3(191.0, 85.0, 236.0) / 255.0;
	}
}

