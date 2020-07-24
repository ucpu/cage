#include <cage-core/assetManager.h>

#include <cage-engine/graphics.h>
#include <cage-engine/sound.h>

namespace cage
{
	namespace detail
	{
		template<> CAGE_API_EXPORT char assetClassId<Font>;
		template<> CAGE_API_EXPORT char assetClassId<Mesh>;
		template<> CAGE_API_EXPORT char assetClassId<RenderObject>;
		template<> CAGE_API_EXPORT char assetClassId<ShaderProgram>;
		template<> CAGE_API_EXPORT char assetClassId<SoundSource>;
		template<> CAGE_API_EXPORT char assetClassId<Texture>;
	}
}
