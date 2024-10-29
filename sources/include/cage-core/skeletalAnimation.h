#ifndef guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
#define guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr

#include <cage-core/core.h>

namespace cage
{
	class Mesh;

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

		void duration(uint64 duration);
		uint64 duration() const;

		void skeletonName(uint32 name);
		uint32 skeletonName() const;
	};

	CAGE_CORE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletalAnimation();
	constexpr uint32 AssetSchemeIndexSkeletalAnimation = 6;

	class CAGE_CORE_API SkeletonRig : private Immovable
	{
	public:
		void clear();
		Holder<SkeletonRig> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void skeletonData(const Mat4 &globalInverse, PointerRange<const uint16> parents, PointerRange<const Mat4> bases, PointerRange<const Mat4> invRests);

		uint32 bonesCount() const;
		Mat4 globalInverse() const;
		PointerRange<const uint16> parents() const;
		PointerRange<const Mat4> bases() const;
		PointerRange<const Mat4> invRests() const;

		// todo sockets
	};

	CAGE_CORE_API Holder<SkeletonRig> newSkeletonRig();

	CAGE_CORE_API AssetsScheme genAssetSchemeSkeletonRig();
	constexpr uint32 AssetSchemeIndexSkeletonRig = 5;

	CAGE_CORE_API void animateSkin(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, PointerRange<Mat4> output); // provides transformation matrices for skinning meshes
	CAGE_CORE_API void animateSkeleton(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, PointerRange<Mat4> output); // provides transformation matrices for individual bones for debug visualization
	CAGE_CORE_API void animateMesh(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, Mesh *mesh);

	namespace detail
	{
		// animationOffset = 0..1 normalized offset, independent of animation speed or duration
		CAGE_CORE_API Real evalCoefficientForSkeletalAnimation(const SkeletalAnimation *animation, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset);
	}
}

#endif // guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
