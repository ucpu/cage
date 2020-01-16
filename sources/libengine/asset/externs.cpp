#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/assetManager.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/sound.h>

namespace cage
{
	namespace detail
	{
		template<> GCHL_API_EXPORT char assetClassId<Font>;
		template<> GCHL_API_EXPORT char assetClassId<Mesh>;
		template<> GCHL_API_EXPORT char assetClassId<RenderObject>;
		template<> GCHL_API_EXPORT char assetClassId<ShaderProgram>;
		template<> GCHL_API_EXPORT char assetClassId<SkeletalAnimation>;
		template<> GCHL_API_EXPORT char assetClassId<SkeletonRig>;
		template<> GCHL_API_EXPORT char assetClassId<SoundSource>;
		template<> GCHL_API_EXPORT char assetClassId<Texture>;
	}
}
