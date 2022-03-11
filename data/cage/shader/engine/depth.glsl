
$include ../shaderConventions.h

$include vertexStage.glsl

$define shader fragment

in vec3 varUv;
in vec3 varPosition; // world space
flat in int varInstanceId;

$include fragmentStage.glsl

void main()
{
	uv = varUv;
	position = varPosition;
	materialLoad();
	if (opacity < 0.5)
		discard;
}
