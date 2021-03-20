#include <cage-core/assetManager.h>

#include <cage-engine/graphics.h>
#include <cage-engine/sound.h>

namespace cage
{
	namespace detail
	{
		template<> CAGE_API_EXPORT char assetClassId<Font>;
		template<> CAGE_API_EXPORT char assetClassId<Model>;
		template<> CAGE_API_EXPORT char assetClassId<RenderObject>;
		template<> CAGE_API_EXPORT char assetClassId<ShaderProgram>;
		template<> CAGE_API_EXPORT char assetClassId<Sound>;
		template<> CAGE_API_EXPORT char assetClassId<Texture>;
	}
}
