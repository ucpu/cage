#ifndef header_skeletalAnimationPreparator_h_hgj4156rsd
#define header_skeletalAnimationPreparator_h_hgj4156rsd

#include <cage-core/mat3x4.h>

namespace cage
{
	class SkeletalAnimation;

	class CAGE_CORE_API SkeletalAnimationPreparatorInstance : private Immovable
	{
	public:
		void prepare(); // thread safe (launches asynchronous task to compute the armature)
		PointerRange<const Mat3x4> armature(); // thread safe, blocking
	};

	class CAGE_CORE_API SkeletalAnimationPreparatorCollection : private Immovable
	{
	public:
		Holder<SkeletalAnimationPreparatorInstance> create(void *object, Holder<SkeletalAnimation> animation, Real coefficient, const Mat4 &modelImportTransform, bool animateSkeletonsInsteadOfSkins = false); // thread safe
		void clear(); // thread safe
	};

	CAGE_CORE_API Holder<SkeletalAnimationPreparatorCollection> newSkeletalAnimationPreparatorCollection(AssetManager *assets);
}

#endif
