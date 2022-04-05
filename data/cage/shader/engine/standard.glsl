
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

layout(early_fragment_tests) in;

void main()
{
	updateNormal();
	Material material = loadMaterial();
	outColor = lighting(material);
}
