
float randomAngle(float freq, vec3 pos)
{
	float dt = dot(floor(pos * freq), vec3(53.1215, 21.1352, 9.1322));
	return fract(sin(dt) * 2105.2354) * 6.283285;
}
