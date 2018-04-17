#include <cage-core/core.h>
#include <cage-core/math.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class skeletonImpl : public skeletonClass
		{
		public:
			skeletonImpl() : mem(detail::systemArena())
			{
				totalBones = namedBones = 0;
				boneParents = nullptr;
				baseMatrices = nullptr;
				invRestMatrices = nullptr;
				namedBoneNames = nullptr;
				namedBoneIndexes = nullptr;
			};

			~skeletonImpl()
			{
				deallocate();
			};

			void deallocate()
			{
				memoryArena &delar = mem;

				delar.deallocate(boneParents);
				delar.deallocate(baseMatrices);
				delar.deallocate(invRestMatrices);
				delar.deallocate(namedBoneNames);
				delar.deallocate(namedBoneIndexes);

				totalBones = namedBones = 0;
				boneParents = nullptr;
				baseMatrices = nullptr;
				invRestMatrices = nullptr;
				namedBoneNames = nullptr;
				namedBoneIndexes = nullptr;
			};

			memoryArena mem;
			uint16 totalBones;
			uint16 namedBones;
			uint16 *boneParents;
			mat4 *baseMatrices;
			mat4 *invRestMatrices;
			uint32 *namedBoneNames;
			uint16 *namedBoneIndexes;
		};
	}

	void skeletonClass::allocate(uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices, uint32 namedBones, const uint32 *namedBoneNames, const uint16 *namedBoneIndexes)
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		impl->deallocate();
		memoryArena &delar = impl->mem;

		impl->totalBones = totalBones;
		impl->namedBones = namedBones;

		impl->boneParents = (uint16*)delar.allocate(impl->totalBones * sizeof(uint16));
		impl->baseMatrices = (mat4*)delar.allocate(impl->totalBones * sizeof(mat4));
		impl->invRestMatrices = (mat4*)delar.allocate(impl->totalBones * sizeof(mat4));
		impl->namedBoneNames = (uint32*)delar.allocate(impl->namedBones * sizeof(uint32));
		impl->namedBoneIndexes = (uint16*)delar.allocate(impl->namedBones * sizeof(uint16));

		detail::memcpy(impl->boneParents, boneParents, impl->totalBones * sizeof(uint16));
		detail::memcpy(impl->baseMatrices, baseMatrices, impl->totalBones * sizeof(mat4));
		detail::memcpy(impl->invRestMatrices, invRestMatrices, impl->totalBones * sizeof(mat4));
		detail::memcpy(impl->namedBoneNames, namedBoneNames, impl->namedBones * sizeof(uint32));
		detail::memcpy(impl->namedBoneIndexes, namedBoneIndexes, impl->namedBones * sizeof(uint16));
	}

	uint32 skeletonClass::bonesCount() const
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		return impl->totalBones;
	}

	uint16 skeletonClass::findNamedBone(uint32 name) const
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		// todo rewrite to binary search
		for (uint32 i = 0; i < impl->namedBones; i++)
		{
			if (impl->namedBoneNames[i] == name)
				return impl->namedBoneIndexes[i];
		}
		return -1;
	}

	void skeletonClass::calculatePose(const animationClass *animation, real coef, mat4 *temporary, mat4 *output)
	{
		CAGE_ASSERT_RUNTIME(coef >= 0 && coef <= 1, coef);
		skeletonImpl *impl = (skeletonImpl*)this;

		for (uint32 i = 0; i < impl->totalBones; i++)
		{
			temporary[i] = impl->baseMatrices[i] * animation->evaluate(i, coef);
			if (impl->boneParents[i] != (uint16)-1)
			{
				CAGE_ASSERT_RUNTIME(impl->boneParents[i] < i, impl->boneParents[i], i);
				temporary[i] = temporary[impl->boneParents[i]] * temporary[i];
			}
			output[i] = temporary[i] * impl->invRestMatrices[i];
		}
	}

	holder<skeletonClass> newSkeleton()
	{
		return detail::systemArena().createImpl <skeletonClass, skeletonImpl>();
	}
}