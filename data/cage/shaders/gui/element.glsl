
layout(std140, set = 2, binding = 0) uniform Element
{
	vec4 posOuter;
	vec4 posInner;
	vec4 accent;
	uint controlType; uint layoutMode; uint dummy1; uint dummy2;
};

struct RectStruct
{
	vec4 outer;
	vec4 inner;
};
struct LayoutStruct
{
	RectStruct modes[4];
};
layout(std430, set = 2, binding = 1) readonly buffer Layouts
{
	LayoutStruct layouts[];
};

layout(location = 1) varying vec2 varUv;


$define shader vertex

layout(location = 0) in vec3 inPosition;

void main()
{
	gl_Position.z = 0;
	gl_Position.w = 1;
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

layout(set = 2, binding = 2) uniform sampler2D texSkin;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(texSkin, varUv);
	outColor.rgb = pow(outColor.rgb, vec3(1.0 / 2.2));
	outColor.rgb = mix(outColor.rgb, accent.rgb, accent.a);
}
