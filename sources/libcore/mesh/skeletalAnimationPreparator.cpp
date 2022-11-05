#include <cage-core/tasks.h>
#include <cage-core/concurrent.h>
#include <cage-core/assetManager.h>
#include <cage-core/memoryAlloca.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/skeletalAnimationPreparator.h>

#include <robin_hood.h>

#include <vector>

namespace cage
{
	namespace
	{
		class SkeletalAnimationPreparatorCollectionImpl : public SkeletalAnimationPreparatorCollection
		{
		public:
			explicit SkeletalAnimationPreparatorCollectionImpl(AssetManager *assets, bool animateSkeletonsInsteadOfSkins) : assets(assets), animateSkeletonsInsteadOfSkins(animateSkeletonsInsteadOfSkins)
			{}

			robin_hood::unordered_map<void *, Holder<class SkeletalAnimationPreparatorInstanceImpl>> objects;
			Holder<Mutex> mutex = newMutex();
			AssetManager *assets = nullptr;
			bool animateSkeletonsInsteadOfSkins = false;
		};

		class SkeletalAnimationPreparatorInstanceImpl : public SkeletalAnimationPreparatorInstance
		{
		public:
			explicit SkeletalAnimationPreparatorInstanceImpl(Holder<SkeletalAnimation> animation, Real coefficient, SkeletalAnimationPreparatorCollectionImpl *impl) : animation(std::move(animation)), coefficient(coefficient), impl(impl)
			{}

			void prepare()
			{
				if (task)
					return;
				ScopeLock lock(impl->mutex);
				if (task)
					return;
				task = tasksRunAsync("skeletal-animation", Holder<SkeletalAnimationPreparatorInstanceImpl>(this, nullptr));
			}

			void operator () (uint32)
			{
				CAGE_ASSERT(armature.empty());
				Holder<SkeletonRig> skeleton = impl->assets->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(animation->skeletonName());
				const uint32 bonesCount = skeleton->bonesCount();
				CAGE_ASSERT(bonesCount > 0);
				CAGE_ASSERT(animation->bonesCount() == bonesCount);
				Mat4 *tmpArmature = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * bonesCount);
				if (impl->animateSkeletonsInsteadOfSkins)
					animateSkeleton(+skeleton, +animation, coefficient, { tmpArmature, tmpArmature + bonesCount });
				else
					animateSkin(+skeleton, +animation, coefficient, { tmpArmature, tmpArmature + bonesCount });
				armature.reserve(bonesCount);
				for (uint32 i = 0; i < bonesCount; i++)
					armature.emplace_back(tmpArmature[i]);
			}

			std::vector<Mat3x4> armature;
			Holder<AsyncTask> task;
			Holder<SkeletalAnimation> animation;
			Real coefficient = Real::Nan();
			SkeletalAnimationPreparatorCollectionImpl *impl = nullptr;
		};
	}

	void SkeletalAnimationPreparatorInstance::prepare()
	{
		SkeletalAnimationPreparatorInstanceImpl *impl = (SkeletalAnimationPreparatorInstanceImpl *)this;
		impl->prepare();
	}

	PointerRange<const Mat3x4> SkeletalAnimationPreparatorInstance::armature()
	{
		SkeletalAnimationPreparatorInstanceImpl *impl = (SkeletalAnimationPreparatorInstanceImpl *)this;
		prepare();
		CAGE_ASSERT(impl->task);
		impl->task->wait();
		return impl->armature;
	}

	Holder<SkeletalAnimationPreparatorInstance> SkeletalAnimationPreparatorCollection::create(void *object, Holder<SkeletalAnimation> animation, Real coefficient)
	{
		SkeletalAnimationPreparatorCollectionImpl *impl = (SkeletalAnimationPreparatorCollectionImpl *)this;
		ScopeLock lock(impl->mutex);
		auto it = impl->objects.find(object);
		if (it != impl->objects.end())
		{
			CAGE_ASSERT(+it->second->animation == +animation);
			CAGE_ASSERT(it->second->coefficient == coefficient);
			return it->second.share().cast<SkeletalAnimationPreparatorInstance>();
		}
		Holder<SkeletalAnimationPreparatorInstanceImpl> inst = systemMemory().createHolder<SkeletalAnimationPreparatorInstanceImpl>(std::move(animation), coefficient, impl);
		impl->objects[object] = inst.share();
		return std::move(inst).cast<SkeletalAnimationPreparatorInstance>();
	}

	void SkeletalAnimationPreparatorCollection::clear()
	{
		SkeletalAnimationPreparatorCollectionImpl *impl = (SkeletalAnimationPreparatorCollectionImpl *)this;
		ScopeLock lock(impl->mutex);
		impl->objects.clear();
	}

	Holder<SkeletalAnimationPreparatorCollection> newSkeletalAnimationPreparatorCollection(AssetManager *assets, bool animateSkeletonsInsteadOfSkins)
	{
		return systemMemory().createImpl<SkeletalAnimationPreparatorCollection, SkeletalAnimationPreparatorCollectionImpl>(assets, animateSkeletonsInsteadOfSkins);
	}
}
