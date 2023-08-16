#ifndef guard_renderObject_h_sd54fgh1ikuj187j4kuj8i
#define guard_renderObject_h_sd54fgh1ikuj187j4kuj8i

#include <cage-engine/core.h>

namespace cage
{
	class CAGE_ENGINE_API RenderObject : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		// lod selection properties

		Real worldSize;
		Real pixelsSize;
		void setLods(PointerRange<const Real> thresholds, PointerRange<const uint32> itemIndices, PointerRange<const uint32> itemNames);
		uint32 lodsCount() const;
		uint32 lodSelect(Real threshold) const;
		PointerRange<const uint32> models(uint32 lod) const;

		// default values for rendering

		Vec3 color = Vec3::Nan();
		Real intensity = Real::Nan();
		Real opacity = Real::Nan();
		Real texAnimSpeed = Real::Nan();
		Real texAnimOffset = Real::Nan();
		uint32 skelAnimName = 0;
		Real skelAnimSpeed = Real::Nan();
		Real skelAnimOffset = Real::Nan();
	};

	CAGE_ENGINE_API Holder<RenderObject> newRenderObject();

	CAGE_ENGINE_API AssetScheme genAssetSchemeRenderObject();
	constexpr uint32 AssetSchemeIndexRenderObject = 13;
}

#endif // guard_renderObject_h_sd54fgh1ikuj187j4kuj8i
