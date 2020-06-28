#include <cage-core/serialization.h>

#include "private.h"

#include <algorithm>
#include <vector>

namespace cage
{
	namespace
	{
		class SkeletalAnimationImpl : public SkeletalAnimation
		{
		public:
			void deallocate()
			{
				indexes.clear();
				positionFrames.clear();
				positionTimes.clear();
				positionValues.clear();
				rotationFrames.clear();
				rotationTimes.clear();
				rotationValues.clear();
				scaleFrames.clear();
				scaleTimes.clear();
				scaleValues.clear();
			}

			uint32 bones = 0;
			std::vector<uint16> indexes;

			std::vector<uint16> positionFrames;
			std::vector<std::vector<real>> positionTimes;
			std::vector<std::vector<vec3>> positionValues;

			std::vector<uint16> rotationFrames;
			std::vector<std::vector<real>> rotationTimes;
			std::vector<std::vector<quat>> rotationValues;

			std::vector<uint16> scaleFrames;
			std::vector<std::vector<real>> scaleTimes;
			std::vector<std::vector<vec3>> scaleValues;

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

	void SkeletalAnimation::deserialize(uint32 bonesCount, PointerRange<const char> buffer)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl*)this;
		impl->deallocate();

		impl->indexes.resize(bonesCount);
		impl->positionFrames.resize(bonesCount);
		impl->rotationFrames.resize(bonesCount);
		impl->scaleFrames.resize(bonesCount);
		impl->positionTimes.resize(bonesCount);
		impl->positionValues.resize(bonesCount);
		impl->rotationTimes.resize(bonesCount);
		impl->rotationValues.resize(bonesCount);
		impl->scaleTimes.resize(bonesCount);
		impl->scaleValues.resize(bonesCount);

		Deserializer des(buffer);
		des.read(bufferCast<char, uint16>(impl->indexes));
		des.read(bufferCast<char, uint16>(impl->positionFrames));
		des.read(bufferCast<char, uint16>(impl->rotationFrames));
		des.read(bufferCast<char, uint16>(impl->scaleFrames));

		for (uint16 b = 0; b < bonesCount; b++)
		{
			if (impl->positionFrames[b])
			{
				impl->positionTimes[b].resize(impl->positionFrames[b]);
				impl->positionValues[b].resize(impl->positionFrames[b]);
				des.read(bufferCast<char, real>(impl->positionTimes[b]));
				des.read(bufferCast<char, vec3>(impl->positionValues[b]));
			}

			if (impl->rotationFrames[b])
			{
				impl->rotationTimes[b].resize(impl->rotationFrames[b]);
				impl->rotationValues[b].resize(impl->rotationFrames[b]);
				des.read(bufferCast<char, real>(impl->rotationTimes[b]));
				des.read(bufferCast<char, quat>(impl->rotationValues[b]));
			}

			if (impl->scaleFrames[b])
			{
				impl->scaleTimes[b].resize(impl->scaleFrames[b]);
				impl->scaleValues[b].resize(impl->scaleFrames[b]);
				des.read(bufferCast<char, real>(impl->scaleTimes[b]));
				des.read(bufferCast<char, vec3>(impl->scaleValues[b]));
			}
		}

		CAGE_ASSERT(des.available() == 0);
	}

	namespace
	{
		real amount(real a, real b, real c)
		{
			CAGE_ASSERT(a <= b);
			if (c < a)
				return 0;
			if (c > b)
				return 1;
			real res = (c - a) / (b - a);
			CAGE_ASSERT(res >= 0 && res <= 1);
			return res;
		}

		uint16 findFrameIndex(real coef, const std::vector<real> &times)
		{
			CAGE_ASSERT(coef >= 0 && coef <= 1);
			CAGE_ASSERT(!times.empty());
			if (coef <= times[0])
				return 0;
			if (coef >= times[times.size() - 1])
				return numeric_cast<uint16>(times.size() - 1);
			auto it = std::lower_bound(times.begin(), times.end(), coef);
			return numeric_cast<uint16>(it - times.begin() - 1);
		}

		template<class Type>
		Type evaluateMatrix(real coef, uint16 frames, const std::vector<real> &times, const std::vector<Type> &values)
		{
			CAGE_ASSERT(frames == times.size());
			switch (frames)
			{
			case 0: return Type();
			case 1: return values[0];
			default:
			{
				uint16 frameIndex = findFrameIndex(coef, times);
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
		CAGE_ASSERT(coef >= 0 && coef <= 1);
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
			uint64 duration = animation->duration;
			if (duration <= 1)
				return 0;
			double sample = ((double)((sint64)emitTime - (sint64)animationStart) * (double)animationSpeed.value + (double)animationOffset.value) / (double)duration;
			// assume that the animation should loop
			return real(sample) % 1;
		}
	}
}
