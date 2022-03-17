
$include ../shaderConventions.h

$include vertexStage.glsl

$include fragmentCommon.glsl

void main()
{
	if (loadMaterial().opacity < 0.5)
		discard;
}
