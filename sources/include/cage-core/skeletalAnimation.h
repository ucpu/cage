#ifndef guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
#define guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr

#include "core.h"

namespace cage
{
	namespace detail
	{
		CAGE_CORE_API real evalCoefficientForSkeletalAnimation(SkeletalAnimation *animation, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	class CAGE_CORE_API SkeletalAnimation : private Immovable
	{
	public:
		uint64 duration = 0;

		void deserialize(uint32 bonesCount, PointerRange<const char> buffer);

		mat4 evaluate(uint16 bone, real coef) const;
	};

	CAGE_CORE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	CAGE_CORE_API AssetScheme genAssetSchemeSkeletalAnimation();
	constexpr uint32 AssetSchemeIndexSkeletalAnimation = 6;

	class CAGE_CORE_API SkeletonRig : private Immovable
	{
	public:
		void deserialize(const mat4 &globalInverse, uint32 bonesCount, PointerRange<const char> buffer);

		uint32 bonesCount() const;
		void animateSkin(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const;
		void animateSkeleton(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const;
	};

	CAGE_CORE_API Holder<SkeletonRig> newSkeletonRig();

	CAGE_CORE_API AssetScheme genAssetSchemeSkeletonRig();
	constexpr uint32 AssetSchemeIndexSkeletonRig = 5;
}

#endif // guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
