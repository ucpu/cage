#ifndef guard_shadowmapFitting_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapFitting_h_FDF074217A66481BBFE8DFD4680D9E01

#include <cage-engine/core.h>

namespace cage
{
	class AssetsManager;
	class Entity;

	struct CAGE_ENGINE_API ShadowmapFittingConfig
	{
		AssetsManager *assets = nullptr;
		Entity *light = nullptr;
		Entity *camera = nullptr; // optional
		uint32 sceneMask = m;
	};

	CAGE_ENGINE_API void shadowmapFitting(const ShadowmapFittingConfig &config);
}

#endif // guard_shadowmapFitting_h_FDF074217A66481BBFE8DFD4680D9E01
