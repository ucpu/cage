
layout(binding = CAGE_SHADER_TEXTURE_DEPTH) uniform sampler2D texGbufferDepth;

vec3 reconstructPosition()
{
	float depth = texelFetch(texGbufferDepth, ivec2(gl_FragCoord.xy), 0).x * 2.0 - 1.0;
	vec4 pos = vec4((gl_FragCoord.xy - uniViewport.xy) / uniViewport.zw * 2.0 - 1.0, depth, 1.0);
	pos = uniVpInv * pos;
	return pos.rgb / pos.w;
}
