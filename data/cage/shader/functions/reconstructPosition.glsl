
vec3 reconstructPosition(sampler2D texDepth, mat4 vpInv, vec4 viewport)
{
	float depth = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x * 2.0 - 1.0;
	vec4 pos = vec4((gl_FragCoord.xy - viewport.xy) / viewport.zw * 2.0 - 1.0, depth, 1.0);
	pos = vpInv * pos;
	return pos.xyz / pos.w;
}
