#ifndef guard_meshIoCommon_h_nbv1ec9rt7zh
#define guard_meshIoCommon_h_nbv1ec9rt7zh

#include <cage-core/mesh.h>

namespace cage
{
	enum class MeshRenderFlags : uint32
	{
		None = 0,
		CutOut = 1 << 0, // leaves, holes in cloth
		Transparent = 1 << 1, // glass, plastic
		Fade = 1 << 2, // holograms, fade out animation
		TwoSided = 1 << 3,
		DepthTest = 1 << 4,
		DepthWrite = 1 << 5,
		ShadowCast = 1 << 6,
		Lighting = 1 << 7,
		Default = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::ShadowCast | MeshRenderFlags::Lighting,
	};
	GCHL_ENUM_BITS(MeshRenderFlags);
}

#endif // guard_meshIoCommon_h_nbv1ec9rt7zh
