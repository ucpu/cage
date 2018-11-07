#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/serialization.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class animationImpl : public animationClass
		{
		public:
			animationImpl() : mem(detail::systemArena())
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

			~animationImpl()
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

			memoryArena mem;

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
				return (uint16)-1;
			}
		};
	}

	void animationClass::allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, const uint16 *scaleFrames, const void *data)
	{
		animationImpl *impl = (animationImpl*)this;
		impl->deallocate();
		impl->duration = duration;
		impl->bones = bones;
		impl->indexes = (uint16*)impl->mem.allocate(sizeof(uint16) * bones);
		impl->positionFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones);
		impl->positionTimes = (real**)impl->mem.allocate(sizeof(real*) * bones);
		impl->positionValues = (vec3**)impl->mem.allocate(sizeof(vec3*) * bones);
		impl->rotationFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones);
		impl->rotationTimes = (real**)impl->mem.allocate(sizeof(real*) * bones);
		impl->rotationValues = (quat**)impl->mem.allocate(sizeof(quat*) * bones);
		impl->scaleFrames = (uint16*)impl->mem.allocate(sizeof(uint16) * bones);
		impl->scaleTimes = (real**)impl->mem.allocate(sizeof(real*) * bones);
		impl->scaleValues = (vec3**)impl->mem.allocate(sizeof(vec3*) * bones);

		detail::memcpy(impl->indexes, indexes, sizeof(uint16) * bones);
		detail::memcpy(impl->positionFrames, positionFrames, sizeof(uint16) * bones);
		detail::memcpy(impl->rotationFrames, rotationFrames, sizeof(uint16) * bones);
		detail::memcpy(impl->scaleFrames, scaleFrames, sizeof(uint16) * bones);

		deserializer des(data, (char*)-1 - (char*)data);
		for (uint16 b = 0; b < bones; b++)
		{
			if (impl->positionFrames[b])
			{
				impl->positionTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->positionFrames[b]);
				des.read(impl->positionTimes[b], positionFrames[b] * sizeof(float));
				impl->positionValues[b] = (vec3*)impl->mem.allocate(sizeof(vec3) * impl->positionFrames[b]);
				des.read(impl->positionValues[b], positionFrames[b] * sizeof(vec3));
			}
			else
			{
				impl->positionTimes[b] = nullptr;
				impl->positionValues[b] = nullptr;
			}

			if (impl->rotationFrames[b])
			{
				impl->rotationTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->rotationFrames[b]);
				des.read(impl->rotationTimes[b], rotationFrames[b] * sizeof(float));
				impl->rotationValues[b] = (quat*)impl->mem.allocate(sizeof(quat) * impl->rotationFrames[b]);
				des.read(impl->rotationValues[b], rotationFrames[b] * sizeof(quat));
			}
			else
			{
				impl->rotationTimes[b] = nullptr;
				impl->rotationValues[b] = nullptr;
			}

			if (impl->scaleFrames[b])
			{
				impl->scaleTimes[b] = (real*)impl->mem.allocate(sizeof(real) * impl->scaleFrames[b]);
				des.read(impl->scaleTimes[b], scaleFrames[b] * sizeof(float));
				impl->scaleValues[b] = (vec3*)impl->mem.allocate(sizeof(vec3) * impl->scaleFrames[b]);
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
			CAGE_ASSERT_RUNTIME(a <= b, a, b, c);
			if (c < a)
				return 0;
			if (c > b)
				return 1;
			real res = (c - a) / (b - a);
			CAGE_ASSERT_RUNTIME(res >= 0 && res <= 1, res, a, b, c);
			return res;
		}

		uint16 findFrameIndex(real coef, const real *times, uint16 length)
		{
			CAGE_ASSERT_RUNTIME(coef >= 0 && coef <= 1, coef);
			CAGE_ASSERT_RUNTIME(length > 0, length);
			// todo rewrite as binary search
			if (coef <= times[0])
				return 0;
			if (coef >= times[length - 1])
				return length - 1;
			for (uint32 i = 0; i + 1 < length; i++)
				if (times[i + 1] > coef)
					return i;
			CAGE_THROW_CRITICAL(exception, "impossible frame index");
		}

		template<class Type> Type evaluateMatrix(real coef, uint16 frames, const real *times, const Type *values)
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

	mat4 animationClass::evaluate(uint16 bone, real coef) const
	{
		CAGE_ASSERT_RUNTIME(coef >= 0 && coef <= 1, coef);
		animationImpl *impl = (animationImpl*)this;

		uint16 b = impl->framesBoneIndex(bone);
		if (b == (uint16)-1)
			return mat4::Nan; // that bone is not animated

		vec3 s = evaluateMatrix(coef, impl->scaleFrames[b], impl->scaleTimes[b], impl->scaleValues[b]);
		mat4 S = mat4(s[0], 0,0,0,0, s[1], 0,0,0,0, s[2], 0,0,0,0, 1);
		mat4 R = mat4(evaluateMatrix(coef, impl->rotationFrames[b], impl->rotationTimes[b], impl->rotationValues[b]));
		mat4 T = mat4(evaluateMatrix(coef, impl->positionFrames[b], impl->positionTimes[b], impl->positionValues[b]));

		return T * R * S;
	}

	uint64 animationClass::duration() const
	{
		animationImpl *impl = (animationImpl*)this;
		return impl->duration;
	}

	holder<animationClass> newAnimation()
	{
		return detail::systemArena().createImpl<animationClass, animationImpl>();
	}

	namespace detail
	{
		real evalCoefficientForSkeletalAnimation(animationClass *animation, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset)
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
