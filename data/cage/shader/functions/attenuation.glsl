
float attenuation(vec3 att, float dist)
{
	return 1 / (att.x + dist * (att.y + dist * att.z));
}
