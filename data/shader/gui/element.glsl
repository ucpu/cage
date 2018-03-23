
$import shaderConventions.h

$define shader vertex

struct RectStruct
{
	vec4 outer;
	vec4 inner;
};

struct LayoutStruct
{
	RectStruct modes[4];
};
layout(std140, binding = 0) uniform Layouts
{
	LayoutStruct layouts[128];
};

layout(location = 0) uniform vec4 posOuter;
layout(location = 1) uniform vec4 posInner;
layout(location = 2) uniform uint controlType;
layout(location = 3) uniform uint layoutMode;
layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;
out vec2 varUv;

void main()
{
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
	switch (int(inPosition.x))
	{
		case 1:
			gl_Position.x = posOuter.x;
			varUv.x = layouts[controlType].modes[layoutMode].outer.x;
		break;
		case 2:
			gl_Position.x = posInner.x;
			varUv.x = layouts[controlType].modes[layoutMode].inner.x;
		break;
		case 3:
			gl_Position.x = posInner.z;
			varUv.x = layouts[controlType].modes[layoutMode].inner.z;
		break;
		case 4:
			gl_Position.x = posOuter.z;
			varUv.x = layouts[controlType].modes[layoutMode].outer.z;
		break;
	}
	switch (int(inPosition.y))
	{
		case 1:
			gl_Position.y = posOuter.y;
			varUv.y = layouts[controlType].modes[layoutMode].outer.y;
		break;
		case 2:
			gl_Position.y = posInner.y;
			varUv.y = layouts[controlType].modes[layoutMode].inner.y;
		break;
		case 3:
			gl_Position.y = posInner.w;
			varUv.y = layouts[controlType].modes[layoutMode].inner.w;
		break;
		case 4:
			gl_Position.y = posOuter.w;
			varUv.y = layouts[controlType].modes[layoutMode].outer.w;
		break;
	}
}

$define shader fragment

layout(binding = 0) uniform sampler2D texSkin;
in vec2 varUv;
out vec4 outColor;

void main()
{
	outColor = texture(texSkin, varUv);
}
