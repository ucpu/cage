
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;
	
    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 lightingBrdf(vec3 light, vec3 L, vec3 V)
{
	vec3 N = normal;
	if (dot(N, N) < 0.5)
		return vec3(0.0); // lighting is disabled

	vec3 H = normalize(L + V);
	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);
	float NoH = max(dot(N, H), 0.0); 
	float VoH = max(dot(V, H), 0.0);

$if 0
	vec3 F0 = mix(vec3(0.04), albedo, metalness);
	vec3 F = fresnelSchlick(NoV, F0);
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;
	
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	
	vec3 nominator = NDF * G * F;
	float denominator = 4.0 * NoV * NoL + 0.001;
	return (kD * albedo / 3.14159 + nominator / denominator) * light * NoL;
$end

$if 0
	vec3 diffuse = albedo * NoL;
	vec3 reflection = mix(vec3(1.0), albedo, metalness);
	vec3 specular = reflection * (pow(NoH, 2.0 / (roughness * roughness) - 2.0) + pow(1.0 - VoH, 5.0));
	return light * (diffuse + specular);
$end

$if 1
	// fresnel term
	float F = pow(1.0 - VoH, 5.0);

	// geometry term
	float x = 2.0 * NoH / max(VoH, 0.00001);
	float G = min(1.0, min(x * NoV, x * NoL));

	// distribution term
	float roughness2 = roughness * roughness;
	float cos2alpha = NoH * NoH;
	float tan2alpha = (cos2alpha - 1.0) / cos2alpha;
	float D = exp(tan2alpha / roughness2) / (3.14159 * roughness2 * cos2alpha * cos2alpha);

	vec3 specular = mix(vec3(1.0), albedo, metalness) * ((F * G * D) / max(NoV * NoL * 3.14159, 0.00001));
	vec3 diffuse = albedo * NoL;
	return light * (diffuse + specular);
$end
}
