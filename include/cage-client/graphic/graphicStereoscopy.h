namespace cage
{
	struct CAGE_API stereoCameraStruct
	{
		vec3 position;
		vec3 direction;
		vec3 worldUp;
		rads fov;
		real near;
		real far;
		real zeroParallaxDistance;
		real eyeSeparation;
		bool ortographic;
		stereoCameraStruct();
	};

	CAGE_API stereoModeEnum stringToStereoMode(const string &mode);
	CAGE_API string stereoModeToString(stereoModeEnum mode);
	CAGE_API void stereoscopy(eyeEnum eye, const stereoCameraStruct &camera, real aspectRatio, stereoModeEnum stereoMode, mat4 &view, mat4 &projection, real &viewportX, real &viewportY, real &viewportWidth, real &viewportHeight);
}
