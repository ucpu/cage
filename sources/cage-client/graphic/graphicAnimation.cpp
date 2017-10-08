#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
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
				rotationFrames = nullptr;
				positionTimes = nullptr;
				rotationTimes = nullptr;
				positionValues = nullptr;
				rotationValues = nullptr;

				bones = 0;
				duration = 0;
			}

			~animationImpl()
			{
				deallocate();
			}

			void deallocate()
			{
				memoryArena &delar = mem;

				for (uint16 b = 0; b < bones; b++)
				{
					delar.deallocate(positionTimes[b]);
					delar.deallocate(positionValues[b]);
					delar.deallocate(rotationTimes[b]);
					delar.deallocate(rotationValues[b]);
				}

				delar.deallocate(indexes);
				delar.deallocate(positionFrames);
				delar.deallocate(rotationFrames);
				delar.deallocate(positionTimes);
				delar.deallocate(rotationTimes);
				delar.deallocate(positionValues);
				delar.deallocate(rotationValues);

				indexes = nullptr;
				positionFrames = nullptr;
				rotationFrames = nullptr;
				positionTimes = nullptr;
				rotationTimes = nullptr;
				positionValues = nullptr;
				rotationValues = nullptr;

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

			const uint16 framesBoneIndex(uint16 boneIndex) const
			{
				// todo rewrite as binary search
				for (uint32 i = 0; i < bones; i++)
					if (indexes[i] == boneIndex)
						return i;
				CAGE_THROW_CRITICAL(exception, "impossible bone index");
			}

			const uint16 frameIndex(real coef, const real *times, uint16 length) const
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
		};
	}

	void animationClass::allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, void *data)
	{
		animationImpl *impl = (animationImpl*)this;
		impl->deallocate();
		memoryArena &delar = impl->mem;

		impl->duration = duration;
		impl->bones = bones;
		impl->indexes = (uint16*)delar.allocate(sizeof(uint16) * bones);
		impl->positionFrames = (uint16*)delar.allocate(sizeof(uint16) * bones);
		impl->rotationFrames = (uint16*)delar.allocate(sizeof(uint16) * bones);
		impl->positionTimes = (real**)delar.allocate(sizeof(real*) * bones);
		impl->positionValues = (vec3**)delar.allocate(sizeof(vec3*) * bones);
		impl->rotationTimes = (real**)delar.allocate(sizeof(real*) * bones);
		impl->rotationValues = (quat**)delar.allocate(sizeof(quat*) * bones);

		detail::memcpy(impl->indexes, indexes, sizeof(uint16) * bones);
		detail::memcpy(impl->positionFrames, positionFrames, sizeof(uint16) * bones);
		detail::memcpy(impl->rotationFrames, rotationFrames, sizeof(uint16) * bones);
		pointer ptr = data;
		for (uint16 b = 0; b < bones; b++)
		{
			if (impl->positionFrames[b])
			{
				impl->positionTimes[b] = (real*)delar.allocate(sizeof(real) * impl->positionFrames[b]);
				detail::memcpy(impl->positionTimes[b], ptr, positionFrames[b] * sizeof(float));
				ptr += positionFrames[b] * sizeof(float);
				impl->positionValues[b] = (vec3*)delar.allocate(sizeof(vec3) * impl->positionFrames[b]);
				detail::memcpy(impl->positionValues[b], ptr, positionFrames[b] * sizeof(vec3));
				ptr += positionFrames[b] * sizeof(vec3);
			}
			else
			{
				impl->positionTimes[b] = nullptr;
				impl->positionValues[b] = nullptr;
			}

			if (impl->rotationFrames[b])
			{
				impl->rotationTimes[b] = (real*)delar.allocate(sizeof(real) * impl->rotationFrames[b]);
				detail::memcpy(impl->rotationTimes[b], ptr, rotationFrames[b] * sizeof(float));
				ptr += rotationFrames[b] * sizeof(float);
				impl->rotationValues[b] = (quat*)delar.allocate(sizeof(quat) * impl->rotationFrames[b]);
				detail::memcpy(impl->rotationValues[b], ptr, rotationFrames[b] * sizeof(quat));
				ptr += rotationFrames[b] * sizeof(quat);
			}
			else
			{
				impl->rotationTimes[b] = nullptr;
				impl->rotationValues[b] = nullptr;
			}
		}
	}

	namespace
	{

		const real amount(real a, real b, real c)
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

		template<class Type> inline const mat4 evaluateMatrix(animationImpl *impl, real coef, uint16 frames, const real *times, const Type *values)
		{
			switch (frames)
			{
			case 0: return mat4();
			case 1: return mat4(values[0]);
			default:
			{
				uint16 frameIndex = impl->frameIndex(coef, times, frames);
				if (frameIndex + 1 == frames)
					return mat4(values[frameIndex]);
				else
				{
					real a = amount(times[frameIndex], times[frameIndex + 1], coef);
					return mat4(interpolate(values[frameIndex], values[frameIndex + 1], a));
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
			return mat4(); // that bone is not animated

		return evaluateMatrix(impl, coef, impl->positionFrames[b], impl->positionTimes[b], impl->positionValues[b])
			* evaluateMatrix(impl, coef, impl->rotationFrames[b], impl->rotationTimes[b], impl->rotationValues[b]);
	}

	uint64 animationClass::duration() const
	{
		animationImpl *impl = (animationImpl*)this;
		return impl->duration;
	}

	real animationClass::coefficient(uint64 time) const
	{
		animationImpl *impl = (animationImpl*)this;
		return (real)(time % impl->duration) / (real)impl->duration;
	}

	holder<animationClass> newAnimation()
	{
		return detail::systemArena().createImpl<animationClass, animationImpl>();
	}
}