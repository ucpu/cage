#include <vector>

#include <unordered_dense.h>

#include <cage-core/assetsManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/memoryAlloca.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/tasks.h>

namespace cage
{
	namespace
	{
		class SkeletalAnimationPreparatorCollectionImpl : public SkeletalAnimationPreparatorCollection
		{
		public:
			explicit SkeletalAnimationPreparatorCollectionImpl(AssetsManager *assets) : assets(assets) {}

			ankerl::unordered_dense::map<void *, Holder<class SkeletalAnimationPreparatorInstanceImpl>> objects;
			Holder<Mutex> mutex = newMutex();
			AssetsManager *assets = nullptr;
		};

		class SkeletalAnimationPreparatorInstanceImpl : public SkeletalAnimationPreparatorInstance
		{
		public:
			explicit SkeletalAnimationPreparatorInstanceImpl(Holder<SkeletalAnimation> animation, Real coefficient, const Mat4 &modelImportTransform, bool animateSkeletonsInsteadOfSkins, SkeletalAnimationPreparatorCollectionImpl *impl) : modelImportTransform(modelImportTransform), animation(std::move(animation)), coefficient(coefficient), animateSkeletonsInsteadOfSkins(animateSkeletonsInsteadOfSkins), impl(impl) {}

			void prepare()
			{
				if (task)
					return;
				ScopeLock lock(impl->mutex);
				if (task)
					return;
				task = tasksRunAsync("skeletal-animation", Holder<SkeletalAnimationPreparatorInstanceImpl>(this, nullptr));
			}

			void operator()(uint32)
			{
				CAGE_ASSERT(armature.empty());
				Holder<SkeletonRig> skeleton = impl->assets->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(animation->skeletonName());
				const uint32 bonesCount = skeleton->bonesCount();
				CAGE_ASSERT(bonesCount > 0);
				CAGE_ASSERT(animation->bonesCount() == bonesCount);
				Mat4 *tmpArmature = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * bonesCount);
				if (animateSkeletonsInsteadOfSkins)
					animateSkeleton(+skeleton, +animation, coefficient, { tmpArmature, tmpArmature + bonesCount });
				else
					animateSkin(+skeleton, +animation, coefficient, { tmpArmature, tmpArmature + bonesCount });
				armature.reserve(bonesCount);
				const Mat4 inv = animateSkeletonsInsteadOfSkins ? Mat4() : inverse(modelImportTransform);
				for (uint32 i = 0; i < bonesCount; i++)
					armature.emplace_back(modelImportTransform * tmpArmature[i] * inv);
			}

			Mat4 modelImportTransform;
			std::vector<Mat3x4> armature;
			Holder<AsyncTask> task;
			Holder<SkeletalAnimation> animation;
			Real coefficient = Real::Nan();
			bool animateSkeletonsInsteadOfSkins = false;
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

	Holder<SkeletalAnimationPreparatorInstance> SkeletalAnimationPreparatorCollection::create(void *object, Holder<SkeletalAnimation> animation, Real coefficient, const Mat4 &modelImportTransform, bool animateSkeletonsInsteadOfSkins)
	{
		SkeletalAnimationPreparatorCollectionImpl *impl = (SkeletalAnimationPreparatorCollectionImpl *)this;
		ScopeLock lock(impl->mutex);
		auto it = impl->objects.find(object);
		if (it != impl->objects.end())
		{
			CAGE_ASSERT(+it->second->animation == +animation);
			CAGE_ASSERT(it->second->coefficient == coefficient);
			CAGE_ASSERT(it->second->animateSkeletonsInsteadOfSkins == animateSkeletonsInsteadOfSkins);
			CAGE_ASSERT(it->second->modelImportTransform == modelImportTransform);
			return it->second.share().cast<SkeletalAnimationPreparatorInstance>();
		}
		Holder<SkeletalAnimationPreparatorInstanceImpl> inst = systemMemory().createHolder<SkeletalAnimationPreparatorInstanceImpl>(std::move(animation), coefficient, modelImportTransform, animateSkeletonsInsteadOfSkins, impl);
		impl->objects[object] = inst.share();
		return std::move(inst).cast<SkeletalAnimationPreparatorInstance>();
	}

	void SkeletalAnimationPreparatorCollection::clear()
	{
		SkeletalAnimationPreparatorCollectionImpl *impl = (SkeletalAnimationPreparatorCollectionImpl *)this;
		ScopeLock lock(impl->mutex);
		impl->objects.clear();
	}

	Holder<SkeletalAnimationPreparatorCollection> newSkeletalAnimationPreparatorCollection(AssetsManager *assets)
	{
		return systemMemory().createImpl<SkeletalAnimationPreparatorCollection, SkeletalAnimationPreparatorCollectionImpl>(assets);
	}
}
