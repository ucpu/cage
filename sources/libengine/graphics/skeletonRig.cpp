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
				boneParents.clear();
				baseMatrices.clear();
				invRestMatrices.clear();
			};

			mat4 globalInverse;
			std::vector<uint16> boneParents;
			std::vector<mat4> baseMatrices;
			std::vector<mat4> invRestMatrices;
			std::vector<mat4> temporary;
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
		return numeric_cast<uint32>(impl->boneParents.size());
	}

	void SkeletonRig::animateSkin(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const
	{
		CAGE_ASSERT(coef >= 0 && coef <= 1);
		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;

		const uint32 totalBones = numeric_cast<uint32>(impl->boneParents.size());
		for (uint32 i = 0; i < totalBones; i++)
		{
			const uint16 p = impl->boneParents[i];
			if (p == m)
				impl->temporary[i] = mat4();
			else
			{
				CAGE_ASSERT(p < i);
				impl->temporary[i] = impl->temporary[p];
			}
			const mat4 anim = animation->evaluate(i, coef);
			impl->temporary[i] = impl->temporary[i] * (anim.valid() ? anim : impl->baseMatrices[i]);
			output[i] = impl->globalInverse * impl->temporary[i] * impl->invRestMatrices[i];
			CAGE_ASSERT(output[i].valid());
		}
	}

	void SkeletonRig::animateSkeleton(const SkeletalAnimation *animation, real coef, PointerRange<mat4> output) const
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl*)this;
		animateSkin(animation, coef, output); // compute temporary

		const auto &pos = [](const mat4 &m)
		{
			return vec3(m * vec4(0, 0, 0, 1));
		};

		const uint32 totalBones = numeric_cast<uint32>(impl->boneParents.size());
		for (uint32 i = 0; i < totalBones; i++)
		{
			const uint16 p = impl->boneParents[i];
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
				{
					vec3 q = abs(dominantAxis(b - a));
					if (distanceSquared(q, vec3(0, 1, 0)) < 0.8)
						q = vec3(1, 0, 0);
					else
						q = vec3(0, 1, 0);
					tr.orientation = quat(b - a, q);
				}
				output[i] = impl->globalInverse * mat4(tr);
				CAGE_ASSERT(output[i].valid());
			}
		}
	}

	Holder<SkeletonRig> newSkeletonRig()
	{
		return detail::systemArena().createImpl<SkeletonRig, SkeletonRigImpl>();
	}
}
