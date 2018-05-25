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
			skeletonImpl() : mem(detail::systemArena()), totalBones(0), boneParents(nullptr), baseMatrices(nullptr), invRestMatrices(nullptr)
			{};

			~skeletonImpl()
			{
				deallocate();
			};

			void deallocate()
			{
				mem.deallocate(boneParents);
				mem.deallocate(baseMatrices);
				mem.deallocate(invRestMatrices);

				totalBones = 0;
				boneParents = nullptr;
				baseMatrices = nullptr;
				invRestMatrices = nullptr;
			};

			memoryArena mem;
			uint16 totalBones;
			uint16 *boneParents;
			mat4 *baseMatrices;
			mat4 *invRestMatrices;
		};
	}

	void skeletonClass::allocate(uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices)
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		impl->deallocate();

		impl->totalBones = totalBones;

		impl->boneParents = (uint16*)impl->mem.allocate(impl->totalBones * sizeof(uint16));
		impl->baseMatrices = (mat4*)impl->mem.allocate(impl->totalBones * sizeof(mat4));
		impl->invRestMatrices = (mat4*)impl->mem.allocate(impl->totalBones * sizeof(mat4));

		detail::memcpy(impl->boneParents, boneParents, impl->totalBones * sizeof(uint16));
		detail::memcpy(impl->baseMatrices, baseMatrices, impl->totalBones * sizeof(mat4));
		detail::memcpy(impl->invRestMatrices, invRestMatrices, impl->totalBones * sizeof(mat4));
	}

	uint32 skeletonClass::bonesCount() const
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		return impl->totalBones;
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
