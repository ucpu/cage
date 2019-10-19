namespace cage
{
	CAGE_API void soundSetSpeakerDirections(uint32 channels, const vec3 *directions);
	CAGE_API void soundGetSpeakerDirections(uint32 channels, vec3 *directions);
	CAGE_API void soundSetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, const vec3 *matrix);
	CAGE_API void soundGetChannelsMixingMatrix(uint32 channelsIn, uint32 channelsOut, vec3 *matrix);
}
