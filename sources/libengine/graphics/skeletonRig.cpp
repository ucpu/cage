#include <cage-core/serialization.h>

#include "private.h"

#include <vector>

namespace cage
{
	namespace
	{
		class SkeletonRigImpl : public SkeletonRig
		{
		public:
			void deallocate()
			{
				totalBones = 0;
				boneParents.clear();
				baseMatrices.clear();
				invRestMatrices.clear();
			};

			mat4 globalInverse;
			std::vector<uint16> boneParents;
			std::vector<mat4> baseMatrices;
			std::vector<mat4> invRestMatrices;
			std::vector<mat4> temporary;
			uint16 totalBones = 0;
		};
	}

	void SkeletonRig::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void SkeletonRig::deserialize(const mat4 &globalInverse, uint32 bonesCount, PointerRange<const char> buffer)
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;
		impl->deallocate();

		impl->globalInverse = globalInverse;
		impl->totalBones = bonesCount;

		impl->boneParents.resize(bonesCount);
		impl->baseMatrices.resize(bonesCount);
		impl->invRestMatrices.resize(bonesCount);

		Deserializer des(buffer);
		des.read(bufferCast<char, uint16>(impl->boneParents));
		des.read(bufferCast<char, mat4>(impl->baseMatrices));
		des.read(bufferCast<char, mat4>(impl->invRestMatrices));
		CAGE_ASSERT(des.available() == 0);

		impl->temporary.resize(bonesCount);
	}

	uint32 SkeletonRig::bonesCount() const
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;
		return impl->totalBones;
	}

	void SkeletonRig::animateSkin(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const
	{
		CAGE_ASSERT(coef >= 0 && coef <= 1);
		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;

		for (uint32 i = 0; i < impl->totalBones; i++)
		{
			uint16 p = impl->boneParents[i];
			if (p == m)
				impl->temporary[i] = mat4();
			else
			{
				CAGE_ASSERT(p < i);
				impl->temporary[i] = impl->temporary[p];
			}
			mat4 anim = animation->evaluate(i, coef);
			impl->temporary[i] = impl->temporary[i] * (anim.valid() ? anim : impl->baseMatrices[i]);
			output[i] = impl->globalInverse * impl->temporary[i] * impl->invRestMatrices[i];
			CAGE_ASSERT(output[i].valid());
		}
	}

	namespace
	{
		vec3 pos(const mat4 &m)
		{
			return vec3(m * vec4(0, 0, 0, 1));
		}
	}

	void SkeletonRig::animateSkeleton(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const
	{
		animateSkin(animation, coef, output); // compute temporary

		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;

		for (uint32 i = 0; i < impl->totalBones; i++)
		{
			uint16 p = impl->boneParents[i];
			if (p == m)
				output[i] = mat4::scale(0); // degenerate
			else
			{
				vec3 a = pos(impl->temporary[p]);
				vec3 b = pos(impl->temporary[i]);
				transform tr;
				tr.position = a;
				tr.scale = distance(a, b);
				if (tr.scale > 0)
					tr.orientation = quat(b - a, vec3(0, 1, 0));
				output[i] = impl->globalInverse * mat4(tr);
			}
			CAGE_ASSERT(output[i].valid());
		}
	}

	Holder<SkeletonRig> newSkeletonRig()
	{
		return detail::systemArena().createImpl<SkeletonRig, SkeletonRigImpl>();
	}
}
