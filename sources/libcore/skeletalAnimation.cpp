#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/mesh.h>

#include <algorithm>
#include <vector>

namespace cage
{
	namespace
	{
		class SkeletalAnimationImpl : public SkeletalAnimation
		{
		public:
			std::vector<uint16> channelsMapping; // channelsMapping[bone] = channel

			std::vector<std::vector<real>> positionTimes;
			std::vector<std::vector<vec3>> positionValues;

			std::vector<std::vector<real>> rotationTimes;
			std::vector<std::vector<quat>> rotationValues;

			std::vector<std::vector<real>> scaleTimes;
			std::vector<std::vector<vec3>> scaleValues;

			uint64 duration = 0;

			static uint16 findFrameIndex(real coef, const std::vector<real> &times)
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

			static real amount(real a, real b, real c)
			{
				CAGE_ASSERT(a <= b);
				if (c < a)
					return 0;
				if (c > b)
					return 1;
				const real res = (c - a) / (b - a);
				CAGE_ASSERT(res >= 0 && res <= 1);
				return res;
			}

			template<class Type>
			static Type evaluateMatrix(real coef, const std::vector<real> &times, const std::vector<Type> &values)
			{
				const uint32 frames = numeric_cast<uint32>(times.size());
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

			mat4 evaluateBone(uint16 bone, real coef, const mat4 &fallback) const
			{
				CAGE_ASSERT(coef >= 0 && coef <= 1);
				CAGE_ASSERT(bone < channelsMapping.size());
				const uint16 ch = channelsMapping[bone];
				if (ch == m)
					return fallback; // this bone is not animated
				const mat4 S = mat4::scale(evaluateMatrix(coef, scaleTimes[ch], scaleValues[ch]));
				const mat4 R = mat4(evaluateMatrix(coef, rotationTimes[ch], rotationValues[ch]));
				const mat4 T = mat4(evaluateMatrix(coef, positionTimes[ch], positionValues[ch]));
				return T * R * S;
			}
		};
	}

	void SkeletalAnimation::clear()
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		impl->channelsMapping.clear();
		impl->positionTimes.clear();
		impl->positionValues.clear();
		impl->rotationTimes.clear();
		impl->rotationValues.clear();
		impl->scaleTimes.clear();
		impl->scaleValues.clear();
	}

	Holder<SkeletalAnimation> SkeletalAnimation::copy() const
	{
		Holder<SkeletalAnimation> res = newSkeletalAnimation();
		res->importBuffer(exportBuffer()); // todo more efficient
		return res;
	}

	namespace
	{
		template<class T>
		Deserializer &operator >> (Deserializer &des, std::vector<T> &vec)
		{
			uint32 cnt = 0;
			des >> cnt;
			std::vector<T> tmp;
			tmp.resize(cnt);
			for (T &it : tmp)
				des >> it;
			std::swap(vec, tmp);
			return des;
		}

		template<class T>
		Serializer &operator << (Serializer &ser, std::vector<T> &vec)
		{
			ser << numeric_cast<uint32>(vec.size());
			for (T &it : vec)
				ser << it;
			return ser;
		}

		template<class T>
		Deserializer &operator << (Deserializer &des, T &other)
		{
			des >> other;
			return des;
		}

		template<class T>
		void serialize(SkeletalAnimationImpl *impl, T &ser)
		{
			ser << impl->channelsMapping;
			ser << impl->positionTimes;
			ser << impl->positionValues;
			ser << impl->rotationTimes;
			ser << impl->rotationValues;
			ser << impl->scaleTimes;
			ser << impl->scaleValues;
			ser << impl->duration;
		}
	}

	Holder<PointerRange<char>> SkeletalAnimation::exportBuffer() const
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		MemoryBuffer buff;
		Serializer ser(buff);
		cage::serialize(impl, ser);
		return PointerRangeHolder<char>(PointerRange<char>(buff));
	}

	void SkeletalAnimation::importBuffer(PointerRange<const char> buffer)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		impl->clear();
		Deserializer des(buffer);
		cage::serialize(impl, des);
		CAGE_ASSERT(des.available() == 0);
	}

	void SkeletalAnimation::channelsMapping(uint16 bones, uint16 channels, PointerRange<const uint16> mapping)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(mapping.size() == bones);
		impl->channelsMapping = std::vector(mapping.begin(), mapping.end());
	}

	namespace
	{
		template<class T>
		void assign(std::vector<std::vector<T>> &dst, PointerRange<const PointerRange<const T>> src)
		{
			dst.clear();
			dst.reserve(src.size());
			for (const auto &it : src)
				dst.push_back(std::vector<T>(it.begin(), it.end()));
		}
	}

	void SkeletalAnimation::positionsData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<real>(impl->positionTimes, times);
		assign<vec3>(impl->positionValues, values);
	}

	void SkeletalAnimation::rotationsData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const quat>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<real>(impl->rotationTimes, times);
		assign<quat>(impl->rotationValues, values);
	}

	void SkeletalAnimation::scaleData(PointerRange<const PointerRange<const real>> times, PointerRange<const PointerRange<const vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<real>(impl->scaleTimes, times);
		assign<vec3>(impl->scaleValues, values);
	}

	uint32 SkeletalAnimation::bonesCount() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return numeric_cast<uint32>(impl->channelsMapping.size());
	}

	uint32 SkeletalAnimation::channelsCount() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return numeric_cast<uint32>(impl->positionTimes.size());
	}

	void SkeletalAnimation::duration(uint64 duration)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		impl->duration = duration;
	}

	uint64 SkeletalAnimation::duration() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return impl->duration;
	}

	Holder<SkeletalAnimation> newSkeletalAnimation()
	{
		return systemArena().createImpl<SkeletalAnimation, SkeletalAnimationImpl>();
	}

	namespace
	{
		class SkeletonRigImpl : public SkeletonRig
		{
		public:
			mat4 globalInverse;
			std::vector<uint16> boneParents;
			std::vector<mat4> baseMatrices;
			std::vector<mat4> invRestMatrices;
		};
	}

	void SkeletonRig::clear()
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		impl->boneParents.clear();
		impl->baseMatrices.clear();
		impl->invRestMatrices.clear();
	}

	Holder<SkeletonRig> SkeletonRig::copy() const
	{
		Holder<SkeletonRig> res = newSkeletonRig();
		res->importBuffer(exportBuffer()); // todo more efficient
		return res;
	}

	namespace
	{
		template<class T>
		void serialize(SkeletonRigImpl *impl, T &ser)
		{
			ser << impl->globalInverse;
			ser << impl->boneParents;
			ser << impl->baseMatrices;
			ser << impl->invRestMatrices;
		}
	}

	Holder<PointerRange<char>> SkeletonRig::exportBuffer() const
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		MemoryBuffer buff;
		Serializer ser(buff);
		cage::serialize(impl, ser);
		return PointerRangeHolder<char>(PointerRange<char>(buff));
	}

	void SkeletonRig::importBuffer(PointerRange<const char> buffer)
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		impl->clear();
		Deserializer des(buffer);
		cage::serialize(impl, des);
		CAGE_ASSERT(impl->boneParents.size() == impl->baseMatrices.size());
		CAGE_ASSERT(impl->boneParents.size() == impl->invRestMatrices.size());
		CAGE_ASSERT(des.available() == 0);
	}

	void SkeletonRig::skeletonData(const mat4 &globalInverse, PointerRange<const uint16> parents, PointerRange<const mat4> bases, PointerRange<const mat4> invRests)
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		impl->clear();
		CAGE_ASSERT(parents.size() == bases.size());
		CAGE_ASSERT(parents.size() == invRests.size());
		impl->globalInverse = globalInverse;
		impl->boneParents = std::vector(parents.begin(), parents.end());
		impl->baseMatrices = std::vector(bases.begin(), bases.end());
		impl->invRestMatrices = std::vector(invRests.begin(), invRests.end());
	}

	uint32 SkeletonRig::bonesCount() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return numeric_cast<uint32>(impl->boneParents.size());
	}

	Holder<SkeletonRig> newSkeletonRig()
	{
		return systemArena().createImpl<SkeletonRig, SkeletonRigImpl>();
	}

	namespace
	{
		void animateTemporary(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, PointerRange<mat4> temporary)
		{
			CAGE_ASSERT(coef >= 0 && coef <= 1);
			const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
			const SkeletalAnimationImpl *anim = (const SkeletalAnimationImpl *)animation;
			const uint32 totalBones = skeleton->bonesCount();
			for (uint32 i = 0; i < totalBones; i++)
			{
				const uint16 p = impl->boneParents[i];
				if (p == m)
					temporary[i] = mat4();
				else
				{
					CAGE_ASSERT(p < i);
					temporary[i] = temporary[p];
				}
				temporary[i] = temporary[i] * anim->evaluateBone(i, coef, impl->baseMatrices[i]);
				CAGE_ASSERT(temporary[i].valid());
			}
		}
	}

	void animateSkin(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, PointerRange<mat4> output)
	{
		CAGE_ASSERT(coef >= 0 && coef <= 1);
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
		const uint32 totalBones = skeleton->bonesCount();
		animateTemporary(skeleton, animation, coef, output);
		for (uint32 i = 0; i < totalBones; i++)
		{
			output[i] = impl->globalInverse * output[i] * impl->invRestMatrices[i];
			CAGE_ASSERT(output[i].valid());
		}
	}

	void animateSkeleton(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, PointerRange<mat4> output)
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
		const uint32 totalBones = skeleton->bonesCount();
		std::vector<mat4> tmp;
		tmp.resize(totalBones);
		animateTemporary(skeleton, animation, coef, tmp);
		for (uint32 i = 0; i < totalBones; i++)
		{
			const uint16 p = impl->boneParents[i];
			if (p == m)
				output[i] = mat4::scale(0); // degenerate
			else
			{
				const vec3 a = vec3(tmp[p] * vec4(0, 0, 0, 1));
				const vec3 b = vec3(tmp[i] * vec4(0, 0, 0, 1));
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

	void animateMesh(const SkeletonRig *skeleton, const SkeletalAnimation *animation, real coef, Mesh *mesh)
	{
		std::vector<mat4> tmp;
		tmp.resize(skeleton->bonesCount());
		animateSkin(skeleton, animation, coef, tmp);
		meshApplyAnimation(mesh, tmp);
	}

	namespace detail
	{
		real evalCoefficientForSkeletalAnimation(const SkeletalAnimation *animation, uint64 currentTime, uint64 startTime, real animationSpeed, real animationOffset)
		{
			if (!animation)
				return 0;
			uint64 duration = animation->duration();
			if (duration <= 1)
				return 0;
			double sample = ((double)((sint64)currentTime - (sint64)startTime) * (double)animationSpeed.value + (double)animationOffset.value) / (double)duration;
			// assume that the animation should loop
			return real(sample - sint64(sample));
		}
	}
}
