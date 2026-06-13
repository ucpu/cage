#ifndef header_skeletalAnimationPreparator_h_hgj4156rsd
#define header_skeletalAnimationPreparator_h_hgj4156rsd

#include <cage-core/mat3x4.h>
#include <cage-core/skeletalAnimation.h>

namespace cage
{
	class SkeletalAnimation;

	struct CAGE_CORE_API SkeletalAnimationPreparatorConfig
	{
		SkeletalAnimationBlendingLayer animations[4];
		Mat4 modelImportTransform;
		void *object = nullptr; // used as unique key
		bool animateSkeletonsInsteadOfSkins = false;
	};

	class CAGE_CORE_API SkeletalAnimationPreparatorInstance : private Immovable
	{
	public:
		void prepare(); // thread safe (launches asynchronous task to compute the armature)
		PointerRange<const Mat3x4> armature(); // thread safe, blocking
	};

	class CAGE_CORE_API SkeletalAnimationPreparatorCollection : private Immovable
	{
	public:
		Holder<SkeletalAnimationPreparatorInstance> create(SkeletalAnimationPreparatorConfig &&config); // thread safe
		void clear(); // thread safe
	};

	CAGE_CORE_API Holder<SkeletalAnimationPreparatorCollection> newSkeletalAnimationPreparatorCollection(AssetsManager *assets);
}

#endif
