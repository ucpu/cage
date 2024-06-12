
float attenuation(vec4 att, float dist)
{
	// att: // type, minDistance, maxDistance, unused
	float minDistance = att[1];
	float maxDistance = att[2];
	dist = clamp(dist, minDistance, maxDistance);
	switch (int(att[0]))
	{
		case 0: // none
			return 1;
		case 1: // linear
			return 1 - (dist - minDistance) / (maxDistance - minDistance);
		case 2: // logarithmic
			return 1 - log(dist - minDistance + 1) / log(maxDistance - minDistance + 1);
		case 3: // inverse square
			return saturate(2 / (1 + sqr(dist / (minDistance + 1))));
		default:
			return 1;
	}
}
