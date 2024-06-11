
vec3 reconstructPosition(sampler2D texDepth, mat4 vpInv, vec4 viewport)
{
	float depth = texelFetch(texDepth, ivec2(gl_FragCoord.xy), 0).x * 2 - 1;
	vec4 pos = vec4((gl_FragCoord.xy - viewport.xy) / viewport.zw * 2 - 1, depth, 1);
	pos = vpInv * pos;
	return pos.xyz / pos.w;
}
