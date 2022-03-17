
$include ../shaderConventions.h

$include vertexStage.glsl

$include fragmentCommon.glsl

out vec4 outColor;

void main()
{
	updateNormal();
	Material material = loadMaterial();
	outColor = lighting(material);
}
