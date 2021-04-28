#ifndef guard_renderObject_h_sd54fgh1ikuj187j4kuj8i
#define guard_renderObject_h_sd54fgh1ikuj187j4kuj8i

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API RenderObject : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		// lod selection properties

		real worldSize;
		real pixelsSize;
		void setLods(PointerRange<const real> thresholds, PointerRange<const uint32> modelIndices, PointerRange<const uint32> modelNames);
		uint32 lodsCount() const;
		uint32 lodSelect(real threshold) const;
		PointerRange<const uint32> models(uint32 lod) const;

		// default values for rendering

		vec3 color = vec3::Nan();
		real intensity = real::Nan();
		real opacity = real::Nan();
		real texAnimSpeed = real::Nan();
		real texAnimOffset = real::Nan();
		uint32 skelAnimName = 0;
		real skelAnimSpeed = real::Nan();
		real skelAnimOffset = real::Nan();
	};

	CAGE_ENGINE_API Holder<RenderObject> newRenderObject();

	CAGE_ENGINE_API AssetScheme genAssetSchemeRenderObject();
	constexpr uint32 AssetSchemeIndexRenderObject = 13;
}

#endif // guard_renderObject_h_sd54fgh1ikuj187j4kuj8i
