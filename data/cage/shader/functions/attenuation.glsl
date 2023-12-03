
float attenuation(vec4 att, float dist)
{
	//return 1 / (att.x + dist * (att.y + dist * (att.z + dist * att.w)));
	return 1 / (att.x + dist * (att.y + dist * att.z));
}
