#ifndef guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
#define guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr

#include "core.h"

namespace cage
{
	class CAGE_CORE_API SkeletalAnimation : private Immovable
	{
	public:
		void clear();
		Holder<SkeletalAnimation> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void channelsMapping(uint16 bones, uint16 channels, PointerRange<const uint16> mapping);
		void positionsData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const vec3>> values);
		void rotationsData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const quat>> values);
		void scaleData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const vec3>> values);

		uint32 bonesCount() const;
		uint32 channelsCount() const;

		void duration(uint64 duration);
		uint64 duration() const;
	};

	CAGE_CORE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	CAGE_CORE_API AssetScheme genAssetSchemeSkeletalAnimation();
	constexpr uint32 AssetSchemeIndexSkeletalAnimation = 6;

	class CAGE_CORE_API SkeletonRig : private Immovable
	{
	public:
		void clear();
		Holder<SkeletonRig> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void skeletonData(const mat4 &globalInverse, PointerRange<const uint16> parents, PointerRange<const mat4> bases, PointerRange<const mat4> invRests);

		uint32 bonesCount() const;

		// todo sockets
	};

	CAGE_CORE_API Holder<SkeletonRig> newSkeletonRig();

	CAGE_CORE_API AssetScheme genAssetSchemeSkeletonRig();
	constexpr uint32 AssetSchemeIndexSkeletonRig = 5;

	CAGE_CORE_API void animateSkin(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, PointerRange<mat4> output); // provides transformation matrices for skinning meshes
	CAGE_CORE_API void animateSkeleton(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, PointerRange<mat4> output); // provides transformation matrices for individual bones for debug visualization
	CAGE_CORE_API void animateMesh(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, Mesh *mesh);

	namespace detail
	{
		CAGE_CORE_API real evalCoefficientForSkeletalAnimation(const SkeletalAnimation *animation, uint64 currentTime, uint64 startTime, real animationSpeed, real animationOffset);
	}
}

#endif // guard_skeletalAnimation_h_dhg4g56efd4km1n56dstfr
