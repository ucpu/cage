
$include ../shaderConventions.h

$include vertex.glsl

$define shader fragment

$include ssaoParams.glsl

layout(binding = 0) uniform sampler2D texAo;

out float outAo;

void main()
{
	float ao = texelFetch(texAo, ivec2(gl_FragCoord.xy), 0).x;
	ao = pow(max(ao - params[1], 0), params[2]) * params[0];
	outAo = clamp(1 - ao, 0, 1);
}
