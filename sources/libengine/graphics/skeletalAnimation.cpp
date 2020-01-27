#include <cage-core/serialization.h>

#include "private.h"

#include <algorithm>

namespace cage
{
	namespace
	{
		class SkeletalAnimationImpl : public SkeletalAnimation
		{
		public:
			SkeletalAnimationImpl() : mem(detail::systemArena())
			{
				indexes = nullptr;
				positionFrames = nullptr;
				positionTimes = nullptr;
				positionValues = nullptr;
				rotationFrames = nullptr;
				rotationTimes = nullptr;
				rotationValues = nullptr;
				scaleFrames = nullptr;
				scaleTimes = nullptr;
				scaleValues = nullptr;

				bones = 0;
				duration = 0;
			}

			~SkeletalAnimationImpl()
			{
				deallocate();
			}

			void deallocate()
			{
				for (uint16 b = 0; b < bones; b++)
				{
					mem.deallocate(positionTimes[b]);
					mem.deallocate(positionValues[b]);
					mem.deallocate(rotationTimes[b]);
					mem.deallocate(rotationValues[b]);
					mem.deallocate(scaleTimes[b]);
					mem.deallocate(scaleValues[b]);
				}

				mem.deallocate(indexes);
				mem.deallocate(positionFrames);
				mem.deallocate(positionTimes);
				mem.deallocate(positionValues);
				mem.deallocate(rotationFrames);
				mem.deallocate(rotationTimes);
				mem.deallocate(rotationValues);
				mem.deallocate(scaleFrames);
				mem.deallocate(scaleTimes);
				mem.deallocate(scaleValues);

				indexes = nullptr;
				positionFrames = nullptr;
				positionTimes = nullptr;
				positionValues = nullptr;
				rotationFrames = nullptr;
				rotationTimes = nullptr;
				rotationValues = nullptr;
				scaleFrames = nullptr;
				scaleTimes = nullptr;
				scaleValues = nullptr;

				bones = 0;
				duration = 0;
			}

			MemoryArena mem;

			uint64 duration;
			uint32 bones;
			uint16 *indexes;

			uint16 *positionFrames;
			real **positionTimes;
			vec3 **positionValues;

			uint16 *rotationFrames;
			real **rotationTimes;
			quat **rotationValues;

			uint16 *scaleFrames;
			real **scaleTimes;
			vec3 **scaleValues;

			uint16 framesBoneIndex(uint16 boneIndex) const
			{
				// todo rewrite as binary search
				for (uint32 i = 0; i < bones; i++)
					if (indexes[i] == boneIndex)
						return i;
				return m;
			}
		};
	}

	void SkeletalAnimation::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void SkeletalAnimation::allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, const uint16 *scaleFrames, const void *data)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl*)this;
		impl->deallocate();
		impl->duration = duration;
		impl->bones = bones;
		impl->indexes = (uint16*)impl->mem.allocate(sizeof(uint16) * bones, sizeof(uintPtr));
		impl->positionFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones, sizeof(uintPtr));
		impl->positionTimes = (real**)impl->mem.allocate(sizeof(real*) * bones, sizeof(uintPtr));
		impl->positionValues = (vec3**)impl->mem.allocate(sizeof(vec3*) * bones, sizeof(uintPtr));
		impl->rotationFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones, sizeof(uintPtr));
		impl->rotationTimes = (real**)impl->mem.allocate(sizeof(real*) * bones, sizeof(uintPtr));
		impl->rotationValues = (quat**)impl->mem.allocate(sizeof(quat*) * bones, sizeof(uintPtr));
		impl->scaleFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones, sizeof(uintPtr));
		impl->scaleTimes = (real**)impl->mem.allocate(sizeof(real*) * bones, sizeof(uintPtr));
		impl->scaleValues = (vec3**)impl->mem.allocate(sizeof(vec3*) * bones, sizeof(uintPtr));

		detail::memcpy(impl->indexes, indexes, sizeof(uint16) * bones);
		detail::memcpy(impl->positionFrames, positionFrames, sizeof(uint16) * bones);
		detail::memcpy(impl->rotationFrames, rotationFrames, sizeof(uint16) * bones);
		detail::memcpy(impl->scaleFrames, scaleFrames, sizeof(uint16) * bones);

		Deserializer des(data, (char*)-1 - (char*)data);
		for (uint16 b = 0; b < bones; b++)
		{
			if (impl->positionFrames[b])
			{
				impl->positionTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->positionFrames[b], sizeof(uintPtr));
				des.read(impl->positionTimes[b], positionFrames[b] * sizeof(float));
				impl->positionValues[b] = (vec3*)impl->mem.allocate(sizeof(vec3) * impl->positionFrames[b], sizeof(uintPtr));
				des.read(impl->positionValues[b], positionFrames[b] * sizeof(vec3));
			}
			else
			{
				impl->positionTimes[b] = nullptr;
				impl->positionValues[b] = nullptr;
			}

			if (impl->rotationFrames[b])
			{
				impl->rotationTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->rotationFrames[b], sizeof(uintPtr));
				des.read(impl->rotationTimes[b], rotationFrames[b] * sizeof(float));
				impl->rotationValues[b] = (quat*)impl->mem.allocate(sizeof(quat) * impl->rotationFrames[b], sizeof(uintPtr));
				des.read(impl->rotationValues[b], rotationFrames[b] * sizeof(quat));
			}
			else
			{
				impl->rotationTimes[b] = nullptr;
				impl->rotationValues[b] = nullptr;
			}

			if (impl->scaleFrames[b])
			{
				impl->scaleTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->scaleFrames[b], sizeof(uintPtr));
				des.read(impl->scaleTimes[b], scaleFrames[b] * sizeof(float));
				impl->scaleValues[b] = (vec3*)impl->mem.allocate(sizeof(vec3) * impl->scaleFrames[b], sizeof(uintPtr));
				des.read(impl->scaleValues[b], scaleFrames[b] * sizeof(vec3));
			}
			else
			{
				impl->scaleTimes[b] = nullptr;
				impl->scaleValues[b] = nullptr;
			}
		}
	}

	namespace
	{
		real amount(real a, real b, real c)
		{
			CAGE_ASSERT(a <= b, a, b, c);
			if (c < a)
				return 0;
			if (c > b)
				return 1;
			real res = (c - a) / (b - a);
			CAGE_ASSERT(res >= 0 && res <= 1, res, a, b, c);
			return res;
		}

		uint16 findFrameIndex(real coef, const real *times, uint16 length)
		{
			CAGE_ASSERT(coef >= 0 && coef <= 1, coef);
			CAGE_ASSERT(length > 0, length);
			if (coef <= times[0])
				return 0;
			if (coef >= times[length - 1])
				return length - 1;
			auto it = std::lower_bound(times, times + length, coef);
			return numeric_cast<uint16>(it - times - 1);
		}

		template<class Type>
		Type evaluateMatrix(real coef, uint16 frames, const real *times, const Type *values)
		{
			switch (frames)
			{
			case 0: return Type();
			case 1: return values[0];
			default:
			{
				uint16 frameIndex = findFrameIndex(coef, times, frames);
				if (frameIndex + 1 == frames)
					return values[frameIndex];
				else
				{
					real a = amount(times[frameIndex], times[frameIndex + 1], coef);
					return interpolate(values[frameIndex], values[frameIndex + 1], a);
				}
			}
			}
		}
	}

	mat4 SkeletalAnimation::evaluate(uint16 bone, real coef) const
	{
		CAGE_ASSERT(coef >= 0 && coef <= 1, coef);
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl*)this;

		uint16 b = impl->framesBoneIndex(bone);
		if (b == m)
			return mat4::Nan(); // that bone is not animated

		vec3 s = evaluateMatrix(coef, impl->scaleFrames[b], impl->scaleTimes[b], impl->scaleValues[b]);
		mat4 S = mat4(s[0], 0,0,0,0, s[1], 0,0,0,0, s[2], 0,0,0,0, 1);
		mat4 R = mat4(evaluateMatrix(coef, impl->rotationFrames[b], impl->rotationTimes[b], impl->rotationValues[b]));
		mat4 T = mat4(evaluateMatrix(coef, impl->positionFrames[b], impl->positionTimes[b], impl->positionValues[b]));

		return T * R * S;
	}

	uint64 SkeletalAnimation::duration() const
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl*)this;
		return impl->duration;
	}

	Holder<SkeletalAnimation> newSkeletalAnimation()
	{
		return detail::systemArena().createImpl<SkeletalAnimation, SkeletalAnimationImpl>();
	}

	namespace detail
	{
		real evalCoefficientForSkeletalAnimation(SkeletalAnimation *animation, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset)
		{
			if (!animation)
				return 0;
			uint64 duration = animation->duration();
			if (duration <= 1)
				return 0;
			double sample = ((double)((sint64)emitTime - (sint64)animationStart) * (double)animationSpeed.value + (double)animationOffset.value) / (double)duration;
			// assume that the animation should loop
			return real(sample) % 1;
		}
	}
}
