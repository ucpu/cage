
$import shaderConventions.h

$define shader vertex

layout(location = CAGE_SHADER_ATTRIB_IN_POSITION) in vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition.xy * 2.0 - 1.0, inPosition.z, 1.0);
}

$define shader fragment

layout(binding = 0) uniform sampler2D texColor;

out vec3 outColor;

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec2 texelSize = 1.0 / textureSize(texColor, 0).xy;
	vec2 uv = gl_FragCoord.xy * texelSize;
	vec3 color = textureLod(texColor, uv, 0).xyz;

	// tone mapping
	//color *= 16.0;
	//float exposureBias = 2.0;
	//vec3 curr = Uncharted2Tonemap(exposureBias * color);
	//vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
	//color = curr * whiteScale;

	// gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	outColor = color;
}
