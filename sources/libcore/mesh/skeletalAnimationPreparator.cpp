#include <vector>

#include <unordered_dense.h>

#include <cage-core/assetsManager.h>
#include <cage-core/assetsSchemes.h>
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
			explicit SkeletalAnimationPreparatorInstanceImpl(SkeletalAnimationPreparatorConfig &&config, SkeletalAnimationPreparatorCollectionImpl *impl) : config(std::move(config)), impl(impl) {}

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
				Holder<SkeletonRig> skeleton;
				for (const auto &it : config.animations)
				{
					if (it.animation)
					{
						skeleton = impl->assets->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(it.animation->skeletonName);
						break;
					}
				}
				if (!skeleton)
					CAGE_THROW_ERROR(Exception, "cannot prepare animations without skeleton");
				const uint32 bonesCount = skeleton->bonesCount();
				CAGE_ASSERT(bonesCount > 0);

				Mat4 *tmpArmature = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * bonesCount);
				PointerRange<Mat4> tmpRange = { tmpArmature, tmpArmature + bonesCount };
				if (config.animateSkeletonsInsteadOfSkins)
					animateSkeleton(+skeleton, config.animations, tmpRange);
				else
					animateSkin(+skeleton, config.animations, tmpRange);

				armature.reserve(bonesCount);
				const Mat4 inv = config.animateSkeletonsInsteadOfSkins ? Mat4() : inverse(config.modelImportTransform);
				for (uint32 i = 0; i < bonesCount; i++)
					armature.emplace_back(config.modelImportTransform * tmpArmature[i] * inv);
			}

			SkeletalAnimationPreparatorConfig config;
			std::vector<Mat3x4> armature;
			Holder<AsyncTask> task;
			SkeletalAnimationPreparatorCollectionImpl *impl = nullptr;
		};

		bool skeletalAnimationConfigSimilarity(const SkeletalAnimationPreparatorConfig &a, const SkeletalAnimationPreparatorConfig &b)
		{
			bool ok = a.animateSkeletonsInsteadOfSkins == b.animateSkeletonsInsteadOfSkins && a.modelImportTransform == b.modelImportTransform;
			for (uint32 i = 0; i < std::extent_v<decltype(SkeletalAnimationPreparatorConfig::animations)>; i++)
			{
				ok &= +a.animations[i].animation == +b.animations[i].animation;
				ok &= a.animations[i].coefficient == b.animations[i].coefficient;
				ok &= a.animations[i].weight == b.animations[i].weight;
			}
			return ok;
		}

		bool validateSkeletalAnimationConfig(const SkeletalAnimationPreparatorConfig &a)
		{
			uint32 skeletonName = 0;
			for (const auto &it : a.animations)
			{
				if (it.animation)
					skeletonName = it.animation->skeletonName;
			}
			if (!skeletonName)
				return false; // at least one animation is present
			for (const auto &it : a.animations)
			{
				if (it.animation && it.animation->skeletonName != skeletonName)
					return false; // all animations must use the same skeleton
			}
			for (const auto &it : a.animations)
			{
				if (it.animation)
				{
					if (!it.coefficient.valid() || it.coefficient != saturate(it.coefficient))
						return false; // coefficient out of range
					if (!it.weight.valid() || it.weight != saturate(it.weight))
						return false; // weight out of range
				}
			}
			return true;
		}
	}

	void SkeletalAnimationPreparatorInstance::prepare()
	{
		SkeletalAnimationPreparatorInstanceImpl *impl = (SkeletalAnimationPreparatorInstanceImpl *)this;
		impl->prepare();
	}

	PointerRange<const Mat3x4> SkeletalAnimationPreparatorInstance::armature()
	{
		SkeletalAnimationPreparatorInstanceImpl *impl = (SkeletalAnimationPreparatorInstanceImpl *)this;
		impl->prepare();
		CAGE_ASSERT(impl->task);
		impl->task->wait();
		return impl->armature;
	}

	Holder<SkeletalAnimationPreparatorInstance> SkeletalAnimationPreparatorCollection::create(SkeletalAnimationPreparatorConfig &&config)
	{
		SkeletalAnimationPreparatorCollectionImpl *impl = (SkeletalAnimationPreparatorCollectionImpl *)this;
		ScopeLock lock(impl->mutex);
		auto it = impl->objects.find(config.object);
		if (it != impl->objects.end())
		{
			CAGE_ASSERT(skeletalAnimationConfigSimilarity(it->second->config, config));
			return it->second.share().cast<SkeletalAnimationPreparatorInstance>();
		}
		CAGE_ASSERT(validateSkeletalAnimationConfig(config));
		Holder<SkeletalAnimationPreparatorInstanceImpl> inst = systemMemory().createHolder<SkeletalAnimationPreparatorInstanceImpl>(std::move(config), impl);
		impl->objects[config.object] = inst.share();
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
