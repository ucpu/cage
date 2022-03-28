
$include ../shaderConventions.h

$include vertex.glsl

void main()
{
	updateVertex();
}

$include fragment.glsl

void main()
{
	if (loadMaterial().opacity < 0.5)
		discard;
}
