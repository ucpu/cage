#ifndef guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
#define guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr

#include <cage-core/math.h>

namespace cage
{
	class Mesh;

	enum class SkeletalAnimationBlendingModeEnum
	{
		Default = 0,
		Override,
		Additive,
	};

	using SkeletalAnimationMaskLabel = detail::StringBase<26>;

	class CAGE_CORE_API SkeletalAnimation : private Immovable
	{
	public:
		void clear();
		Holder<SkeletalAnimation> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void channelsMapping(uint16 bones, uint16 channels, PointerRange<const uint16> mapping);
		void positionsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values);
		void rotationsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Quat>> values);
		void scaleData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values);

		uint32 bonesCount() const;
		uint32 channelsCount() const;

		uint64 duration = 0;
		uint32 skeletonName = 0;
		SkeletalAnimationMaskLabel defaultMaskName;
		SkeletalAnimationBlendingModeEnum defaultBlendingMode = SkeletalAnimationBlendingModeEnum::Override;
	};

	CAGE_CORE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	class CAGE_CORE_API SkeletonRig : private Immovable
	{
	public:
		void clear();
		Holder<SkeletonRig> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void skeletonData(PointerRange<const uint16> parents, PointerRange<const Mat4> bases, PointerRange<const Mat4> invRests);
		void namedMask(const SkeletalAnimationMaskLabel &name, PointerRange<const Real> mask);

		uint32 bonesCount() const;
		PointerRange<const Real> namedMask(const SkeletalAnimationMaskLabel &name) const;

		Mat4 globalInverse;
	};

	CAGE_CORE_API Holder<SkeletonRig> newSkeletonRig();

	struct CAGE_CORE_API SkeletalAnimationBlendingLayer
	{
		PointerRange<const Real> mask;
		const SkeletalAnimation *animation = nullptr;
		Real coefficient;
		Real weight = 1;
		SkeletalAnimationBlendingModeEnum blendingMode = SkeletalAnimationBlendingModeEnum::Override;
	};

	CAGE_CORE_API void animateSkin(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, PointerRange<Mat4> output); // provides transformation matrices for skinning meshes
	CAGE_CORE_API void animateSkeleton(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, PointerRange<Mat4> output); // provides transformation matrices for individual bones for debug visualization
	CAGE_CORE_API void animateMesh(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, Mesh *mesh);

	namespace detail
	{
		// animationOffset = 0..1 normalized offset, independent of animation speed or duration
		CAGE_CORE_API Real evalCoefficientForSkeletalAnimation(const SkeletalAnimation *animation, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset);
	}
}

#endif // guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
