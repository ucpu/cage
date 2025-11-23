#include <algorithm>
#include <vector>

#include <cage-core/containerSerialization.h>
#include <cage-core/math.h>
#include <cage-core/memoryAlloca.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/skeletalAnimation.h>

namespace cage
{
	namespace
	{
		class SkeletalAnimationImpl : public SkeletalAnimation
		{
		public:
			std::vector<uint16> channelsMapping; // channelsMapping[bone] = channel

			std::vector<std::vector<Real>> positionTimes;
			std::vector<std::vector<Vec3>> positionValues;

			std::vector<std::vector<Real>> rotationTimes;
			std::vector<std::vector<Quat>> rotationValues;

			std::vector<std::vector<Real>> scaleTimes;
			std::vector<std::vector<Vec3>> scaleValues;

			uint64 duration = 0;
			uint32 skeletonName = 0;

			CAGE_FORCE_INLINE static uint16 findFrameIndex(Real coef, const std::vector<Real> &times)
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

			CAGE_FORCE_INLINE static Real amount(Real a, Real b, Real c)
			{
				CAGE_ASSERT(a <= b);
				if (c < a)
					return 0;
				if (c > b)
					return 1;
				const Real res = (c - a) / (b - a);
				CAGE_ASSERT(res >= 0 && res <= 1);
				return res;
			}

			template<class Type>
			CAGE_FORCE_INLINE static Type evaluateMatrix(Real coef, const std::vector<Real> &times, const std::vector<Type> &values)
			{
				const uint16 frames = numeric_cast<uint16>(times.size());
				switch (frames)
				{
					case 0:
						return Type();
					case 1:
						return values[0];
					default:
					{
						const uint16 frameIndex = findFrameIndex(coef, times);
						if (frameIndex + 1 == frames)
							return values[frameIndex];
						else
						{
							const Real a = amount(times[frameIndex], times[frameIndex + 1], coef);
							return interpolate(values[frameIndex], values[frameIndex + 1], a);
						}
					}
				}
			}

			Mat4 evaluateBone(uint16 bone, Real coef, const Mat4 &fallback) const
			{
				CAGE_ASSERT(coef >= 0 && coef <= 1);
				CAGE_ASSERT(bone < channelsMapping.size());
				const uint16 ch = channelsMapping[bone];
				if (ch == m)
					return fallback; // this bone is not animated
				const Mat4 S = Mat4::scale(evaluateMatrix(coef, scaleTimes[ch], scaleValues[ch]));
				const Mat4 R = Mat4(evaluateMatrix(coef, rotationTimes[ch], rotationValues[ch]));
				const Mat4 T = Mat4(evaluateMatrix(coef, positionTimes[ch], positionValues[ch]));
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
		CAGE_FORCE_INLINE Deserializer &operator<<(Deserializer &des, T &other)
		{
			des >> other;
			return des;
		}

		template<class T>
		CAGE_FORCE_INLINE void serialize(SkeletalAnimationImpl *impl, T &ser)
		{
			ser << impl->channelsMapping;
			ser << impl->positionTimes;
			ser << impl->positionValues;
			ser << impl->rotationTimes;
			ser << impl->rotationValues;
			ser << impl->scaleTimes;
			ser << impl->scaleValues;
			ser << impl->duration;
			ser << impl->skeletonName;
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
		CAGE_FORCE_INLINE void assign(std::vector<std::vector<T>> &dst, PointerRange<const PointerRange<const T>> src)
		{
			dst.clear();
			dst.reserve(src.size());
			for (const auto &it : src)
				dst.push_back(std::vector<T>(it.begin(), it.end()));
		}
	}

	void SkeletalAnimation::positionsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real>(impl->positionTimes, times);
		assign<Vec3>(impl->positionValues, values);
	}

	void SkeletalAnimation::rotationsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Quat>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real>(impl->rotationTimes, times);
		assign<Quat>(impl->rotationValues, values);
	}

	void SkeletalAnimation::scaleData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real>(impl->scaleTimes, times);
		assign<Vec3>(impl->scaleValues, values);
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

	void SkeletalAnimation::skeletonName(uint32 name)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		impl->skeletonName = name;
	}

	uint32 SkeletalAnimation::skeletonName() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return impl->skeletonName;
	}

	Holder<SkeletalAnimation> newSkeletalAnimation()
	{
		return systemMemory().createImpl<SkeletalAnimation, SkeletalAnimationImpl>();
	}

	namespace
	{
		class SkeletonRigImpl : public SkeletonRig
		{
		public:
			Mat4 globalInverse;
			std::vector<uint16> boneParents;
			std::vector<Mat4> baseMatrices;
			std::vector<Mat4> invRestMatrices;
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
		CAGE_FORCE_INLINE void serialize(SkeletonRigImpl *impl, T &ser)
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

	void SkeletonRig::skeletonData(const Mat4 &globalInverse, PointerRange<const uint16> parents, PointerRange<const Mat4> bases, PointerRange<const Mat4> invRests)
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

	Mat4 SkeletonRig::globalInverse() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return impl->globalInverse;
	}

	PointerRange<const uint16> SkeletonRig::parents() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return impl->boneParents;
	}

	PointerRange<const Mat4> SkeletonRig::bases() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return impl->baseMatrices;
	}

	PointerRange<const Mat4> SkeletonRig::invRests() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return impl->invRestMatrices;
	}

	Holder<SkeletonRig> newSkeletonRig()
	{
		return systemMemory().createImpl<SkeletonRig, SkeletonRigImpl>();
	}

	namespace
	{
		void animateTemporary(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, PointerRange<Mat4> temporary)
		{
			CAGE_ASSERT(temporary.size() == skeleton->bonesCount());
			CAGE_ASSERT(coef >= 0 && coef <= 1);
			const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
			const SkeletalAnimationImpl *anim = (const SkeletalAnimationImpl *)animation;
			const uint32 totalBones = skeleton->bonesCount();
			for (uint32 i = 0; i < totalBones; i++)
			{
				const uint16 p = impl->boneParents[i];
				if (p == m)
					temporary[i] = Mat4();
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

	void animateSkin(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, PointerRange<Mat4> output)
	{
		CAGE_ASSERT(output.size() == skeleton->bonesCount());
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

	void animateSkeleton(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, PointerRange<Mat4> output)
	{
		CAGE_ASSERT(output.size() == skeleton->bonesCount());
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
		const uint32 totalBones = skeleton->bonesCount();
		Mat4 *tmp = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * totalBones);
		animateTemporary(skeleton, animation, coef, { tmp, tmp + totalBones });
		for (uint32 i = 0; i < totalBones; i++)
		{
			const uint16 p = impl->boneParents[i];
			if (p == m)
				output[i] = Mat4::scale(0); // degenerate
			else
			{
				const Vec3 a = Vec3(tmp[p] * Vec4(0, 0, 0, 1));
				const Vec3 b = Vec3(tmp[i] * Vec4(0, 0, 0, 1));
				Transform tr;
				tr.position = a;
				tr.scale = distance(a, b);
				if (tr.scale > 0)
				{
					Vec3 q = abs(dominantAxis(b - a));
					if (distanceSquared(q, Vec3(0, 1, 0)) < 0.8)
						q = Vec3(1, 0, 0);
					else
						q = Vec3(0, 1, 0);
					tr.orientation = Quat(b - a, q);
				}
				output[i] = impl->globalInverse * Mat4(tr);
				CAGE_ASSERT(output[i].valid());
			}
		}
	}

	void animateMesh(const SkeletonRig *skeleton, const SkeletalAnimation *animation, Real coef, Mesh *mesh)
	{
		const uint32 totalBones = skeleton->bonesCount();
		Mat4 *tmp = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * totalBones);
		animateSkin(skeleton, animation, coef, { tmp, tmp + totalBones });
		meshApplyAnimation(mesh, { tmp, tmp + totalBones });
	}

	namespace detail
	{
		Real evalCoefficientForSkeletalAnimation(const SkeletalAnimation *animation, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset)
		{
			if (!animation)
				return 0;
			const uint64 duration = max(animation->duration(), uint64(1));
			const double sample = ((double)((sint64)currentTime - (sint64)startTime) * (double)animationSpeed.value) / (double)duration + (double)animationOffset.value;
			// assume that the animation should loop
			return saturate(Real(sample - sint64(sample)));
		}
	}
}
