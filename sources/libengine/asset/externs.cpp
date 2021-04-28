#include <cage-core/assetManager.h>

#include <cage-engine/core.h>

namespace cage
{
	namespace detail
	{
		template<> CAGE_API_EXPORT char assetTypeBlock<Font>;
		template<> CAGE_API_EXPORT char assetTypeBlock<Model>;
		template<> CAGE_API_EXPORT char assetTypeBlock<RenderObject>;
		template<> CAGE_API_EXPORT char assetTypeBlock<ShaderProgram>;
		template<> CAGE_API_EXPORT char assetTypeBlock<Sound>;
		template<> CAGE_API_EXPORT char assetTypeBlock<Texture>;
	}
}
