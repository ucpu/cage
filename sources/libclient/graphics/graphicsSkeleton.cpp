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
			mat4 globalInverse;
			uint16 totalBones;
			uint16 *boneParents;
			mat4 *baseMatrices;
			mat4 *invRestMatrices;
		};
	}

	void skeletonClass::allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices)
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		impl->deallocate();

		impl->globalInverse = globalInverse;
		impl->totalBones = totalBones;

		impl->boneParents = (uint16*)impl->mem.allocate(impl->totalBones * sizeof(uint16), sizeof(uintPtr));
		impl->baseMatrices = (mat4*)impl->mem.allocate(impl->totalBones * sizeof(mat4), sizeof(uintPtr));
		impl->invRestMatrices = (mat4*)impl->mem.allocate(impl->totalBones * sizeof(mat4), sizeof(uintPtr));

		detail::memcpy(impl->boneParents, boneParents, impl->totalBones * sizeof(uint16));
		detail::memcpy(impl->baseMatrices, baseMatrices, impl->totalBones * sizeof(mat4));
		detail::memcpy(impl->invRestMatrices, invRestMatrices, impl->totalBones * sizeof(mat4));
	}

	uint32 skeletonClass::bonesCount() const
	{
		skeletonImpl *impl = (skeletonImpl*)this;
		return impl->totalBones;
	}

	void skeletonClass::animateSkin(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const
	{
		CAGE_ASSERT_RUNTIME(coef >= 0 && coef <= 1, coef);
		skeletonImpl *impl = (skeletonImpl*)this;

		for (uint32 i = 0; i < impl->totalBones; i++)
		{
			uint16 p = impl->boneParents[i];
			if (p == (uint16)-1)
				temporary[i] = mat4();
			else
			{
				CAGE_ASSERT_RUNTIME(p < i, p, i);
				temporary[i] = temporary[p];
			}
			mat4 anim = animation->evaluate(i, coef);
			temporary[i] = temporary[i] * (anim.valid() ? anim : impl->baseMatrices[i]);
			output[i] = impl->globalInverse * temporary[i] * impl->invRestMatrices[i];
			CAGE_ASSERT_RUNTIME(output[i].valid(), output[i], i);
		}
	}

	namespace
	{
		vec3 pos(const mat4 &m)
		{
			return vec3(m * vec4(0, 0, 0, 1));
		}
	}

	void skeletonClass::animateSkeleton(const animationClass *animation, real coef, mat4 *temporary, mat4 *output) const
	{
		animateSkin(animation, coef, temporary, output); // compute temporary

		skeletonImpl *impl = (skeletonImpl*)this;

		for (uint32 i = 0; i < impl->totalBones; i++)
		{
			uint16 p = impl->boneParents[i];
			if (p == (uint16)-1)
				output[i] = mat4(0); // degenerate
			else
			{
				vec3 a = pos(temporary[p]);
				vec3 b = pos(temporary[i]);
				transform tr;
				tr.position = a;
				tr.scale = a.distance(b);
				if (tr.scale > 0)
					tr.orientation = quat(b - a, vec3(0, 1, 0));
				output[i] = impl->globalInverse * mat4(tr);
			}
			CAGE_ASSERT_RUNTIME(output[i].valid(), output[i], i);
		}
	}

	holder<skeletonClass> newSkeleton()
	{
		return detail::systemArena().createImpl<skeletonClass, skeletonImpl>();
	}
}
