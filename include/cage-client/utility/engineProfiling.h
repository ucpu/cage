#ifndef guard_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2

namespace cage
{
	enum class profilingModeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class CAGE_API engineProfilingClass
	{
	public:
		vec2 screenPosition;
		uint32 keyToggleProfilingMode;
		uint32 keyVisualizeBufferPrev;
		uint32 keyVisualizeBufferNext;
		uint32 keyToggleRenderMissingMeshes;
		uint32 keyToggleStereo;
		uint32 keyToggleFullscreen;
		modifiersFlags keyModifiers;
		profilingModeEnum profilingMode;
	};

	CAGE_API holder<engineProfilingClass> newEngineProfiling();
}

#endif // guard_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
