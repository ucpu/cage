#ifndef guard_meshIoCommon_h_nbv1ec9rt7zh
#define guard_meshIoCommon_h_nbv1ec9rt7zh

#include <cage-core/enumBits.h>
#include <cage-core/mesh.h>

namespace cage
{
	enum class MeshRenderFlags : uint32
	{
		None = 0,
		CutOut = 1 << 0, // leaves, holes in cloth (no blending, but prevents early-z)
		Transparent = 1 << 1, // glass, plastic (alpha blending, with specular highlights)
		Fade = 1 << 2, // holograms, fade out animation (alpha blending, no highlights)
		OrderIndependent = 1 << 3, // opaque, cutout (prevents order-dependent for transparent, fade, or opacity<1 from the application)
		TwoSided = 1 << 4,
		DepthTest = 1 << 5,
		DepthWrite = 1 << 6,
		ShadowCast = 1 << 7,
		Lighting = 1 << 8,
		Default = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::ShadowCast | MeshRenderFlags::Lighting,
	};
	CAGE_ENUM_BITS(MeshRenderFlags);
}

#endif // guard_meshIoCommon_h_nbv1ec9rt7zh
