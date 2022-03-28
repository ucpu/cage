
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

out vec4 outColor;

void main()
{
	updateNormal();
	Material material = loadMaterial();
	if (material.opacity < 0.5)
		discard;
	outColor = lighting(material);
}
