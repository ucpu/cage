#ifndef guard_meshMaterial_h_dgf54ik1g8r
#define guard_meshMaterial_h_dgf54ik1g8r

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API MeshMaterial
	{
		// albedo rgb is linear, and NOT alpha-premultiplied
		Vec4 albedoBase = Vec4(0);
		Vec4 specialBase = Vec4(0);
		Vec4 albedoMult = Vec4(1);
		Vec4 specialMult = Vec4(1);
	};

	enum class MeshRenderFlags : uint32
	{
		None = 0,
		Translucent = 1 << 1,
		TwoSided = 1 << 2,
		DepthTest = 1 << 3,
		DepthWrite = 1 << 4,
		ShadowCast = 1 << 5,
		Lighting = 1 << 6,
	};
	GCHL_ENUM_BITS(MeshRenderFlags);

	enum class MeshTextureType : uint32
	{
		None = 0,
		Albedo,
		Special,
		Normal,
	};

	namespace detail
	{
		CAGE_CORE_API StringLiteral meshTextureTypeToString(MeshTextureType type);
	}
}

#endif // guard_meshMaterial_h_dgf54ik1g8r
