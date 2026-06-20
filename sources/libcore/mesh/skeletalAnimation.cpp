#include <algorithm>
#include <unordered_map>
#include <vector>

#include <cage-core/containerSerialization.h>
#include <cage-core/math.h>
#include <cage-core/memoryAlloca.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/stdHash.h>

namespace cage
{
	namespace
	{
		// transform with non-uniform scale
		struct Trs3
		{
			Quat r;
			Vec3 t;
			Vec3 s = Vec3(1);
		};

		// animation data for a single bone
		struct Channel
		{
			std::vector<Real> positionTimes;
			std::vector<Vec3> positionValues;
			std::vector<Real> rotationTimes;
			std::vector<Quat> rotationValues;
			std::vector<Real> scaleTimes;
			std::vector<Vec3> scaleValues;
		};

		class SkeletalAnimationImpl : public SkeletalAnimation
		{
		public:
			// channels represent a subset of bones that have actual animation data
			std::vector<uint16> channelsMapping; // channelsMapping[bone] = channel
			std::vector<Channel> channels;

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
			CAGE_FORCE_INLINE static Type sampleChannel(Real coef, const std::vector<Real> &times, const std::vector<Type> &values, const Type &fallback)
			{
				const uint16 frames = numeric_cast<uint16>(times.size());
				switch (frames)
				{
					case 0:
						return fallback;
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

			Trs3 evaluateBone(uint16 bone, Real coef, const Trs3 &fallback) const
			{
				CAGE_ASSERT(coef >= 0 && coef <= 1);
				CAGE_ASSERT(bone < channelsMapping.size());
				const uint16 ch = channelsMapping[bone];
				if (ch == m)
					return fallback; // this bone is not animated
				const Channel &c = channels[ch];
				Trs3 res;
				res.r = sampleChannel(coef, c.rotationTimes, c.rotationValues, fallback.r);
				res.t = sampleChannel(coef, c.positionTimes, c.positionValues, fallback.t);
				res.s = sampleChannel(coef, c.scaleTimes, c.scaleValues, fallback.s);
				return res;
			}
		};
	}

	void SkeletalAnimation::clear()
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		impl->channelsMapping.clear();
		impl->channels.clear();
		impl->maskName = {};
		impl->duration = 0;
		impl->skeletonName = 0;
		impl->blendingMode = SkeletalAnimationBlendingModeFlags::Default;
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
		CAGE_FORCE_INLINE void serialize(Channel &ch, T &ser)
		{
			ser << ch.positionTimes;
			ser << ch.positionValues;
			ser << ch.rotationTimes;
			ser << ch.rotationValues;
			ser << ch.scaleTimes;
			ser << ch.scaleValues;
		}

		CAGE_FORCE_INLINE Serializer &operator<<(Serializer &ser, const Channel &other)
		{
			serialize(const_cast<Channel &>(other), ser);
			return ser;
		}

		CAGE_FORCE_INLINE Deserializer &operator>>(Deserializer &des, Channel &other)
		{
			serialize(other, des);
			return des;
		}

		template<class T>
		CAGE_FORCE_INLINE void serialize(SkeletalAnimationImpl *impl, T &ser)
		{
			ser << impl->channelsMapping;
			ser << impl->channels;
			ser << impl->maskName;
			ser << impl->duration;
			ser << impl->skeletonName;
			ser << impl->blendingMode;
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
		impl->channels.clear();
		impl->channels.resize(channels);
	}

	namespace
	{
		template<class T, auto Accessor>
		CAGE_FORCE_INLINE void assign(std::vector<Channel> &channels, PointerRange<const PointerRange<const T>> src)
		{
			CAGE_ASSERT(channels.size() == src.size());
			for (uint32 i = 0; i < src.size(); ++i)
			{
				auto &dst = channels[i].*Accessor;
				const auto &range = src[i];
				dst.assign(range.begin(), range.end());
			}
		}
	}

	void SkeletalAnimation::positionsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real, &Channel::positionTimes>(impl->channels, times);
		assign<Vec3, &Channel::positionValues>(impl->channels, values);
	}

	void SkeletalAnimation::rotationsData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Quat>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real, &Channel::rotationTimes>(impl->channels, times);
		assign<Quat, &Channel::rotationValues>(impl->channels, values);
	}

	void SkeletalAnimation::scaleData(PointerRange<const PointerRange<const Real>> times, PointerRange<const PointerRange<const Vec3>> values)
	{
		SkeletalAnimationImpl *impl = (SkeletalAnimationImpl *)this;
		CAGE_ASSERT(times.size() == values.size());
		assign<Real, &Channel::scaleTimes>(impl->channels, times);
		assign<Vec3, &Channel::scaleValues>(impl->channels, values);
	}

	uint32 SkeletalAnimation::bonesCount() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return numeric_cast<uint32>(impl->channelsMapping.size());
	}

	uint32 SkeletalAnimation::channelsCount() const
	{
		const SkeletalAnimationImpl *impl = (const SkeletalAnimationImpl *)this;
		return numeric_cast<uint32>(impl->channels.size());
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
			std::vector<uint16> boneParents;
			std::vector<Trs3> baseMatrices;
			std::vector<Mat4> invRestMatrices;
			std::unordered_map<SkeletalAnimationMaskLabel, std::vector<Real>> masksMapping;
		};
	}

	void SkeletonRig::clear()
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		impl->boneParents.clear();
		impl->baseMatrices.clear();
		impl->invRestMatrices.clear();
		impl->masksMapping.clear();
		impl->globalInverse = {};
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
			ser << impl->boneParents;
			ser << impl->baseMatrices;
			ser << impl->invRestMatrices;
			ser << impl->masksMapping;
			ser << impl->globalInverse;
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

	namespace
	{
		Trs3 decompose3(const Mat4 &v)
		{
			Trs3 res;
			decompose(v, res.t, res.r, res.s);
			return res;
		}
	}

	void SkeletonRig::skeletonData(PointerRange<const uint16> parents, PointerRange<const Mat4> bases, PointerRange<const Mat4> invRests)
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		impl->clear();
		CAGE_ASSERT(parents.size() == bases.size());
		CAGE_ASSERT(parents.size() == invRests.size());
		impl->boneParents = std::vector(parents.begin(), parents.end());
		impl->baseMatrices.reserve(bases.size());
		for (const Mat4 &b : bases)
			impl->baseMatrices.push_back(decompose3(b));
		impl->invRestMatrices = std::vector(invRests.begin(), invRests.end());
	}

	void SkeletonRig::namedMask(const SkeletalAnimationMaskLabel &name, PointerRange<const Real> mask)
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		CAGE_ASSERT(mask.size() == bonesCount());
		impl->masksMapping[name].assign(mask.begin(), mask.end());
	}

	uint32 SkeletonRig::bonesCount() const
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)this;
		return numeric_cast<uint32>(impl->boneParents.size());
	}

	PointerRange<const Real> SkeletonRig::namedMask(const SkeletalAnimationMaskLabel &name) const
	{
		SkeletonRigImpl *impl = (SkeletonRigImpl *)this;
		const auto it = impl->masksMapping.find(name);
		if (it != impl->masksMapping.end())
			return it->second;
		return {};
	}

	Holder<SkeletonRig> newSkeletonRig()
	{
		return systemMemory().createImpl<SkeletonRig, SkeletonRigImpl>();
	}

	namespace
	{
		CAGE_FORCE_INLINE Trs3 overrideBlending(const Trs3 &a, const Trs3 &b, Real w)
		{
			Trs3 r;
			r.r = interpolate(a.r, b.r, w);
			r.t = interpolate(a.t, b.t, w);
			r.s = interpolate(a.s, b.s, w);
			return r;
		}

		CAGE_FORCE_INLINE Trs3 additiveBlending(const Trs3 &base, const Trs3 &sample, const Trs3 &reference, Real w)
		{
			Trs3 r;
			r.r = base.r * interpolate(Quat(), conjugate(reference.r) * sample.r, w);
			r.t = base.t + (sample.t - reference.t) * w;
			r.s = base.s + (sample.s - reference.s) * w;
			return r;
		}

		CAGE_FORCE_INLINE Mat4 convert(Trs3 x)
		{
			return Mat4(x.t, x.r, x.s);
		}

		void animateImpl(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, PointerRange<Mat4> temporary)
		{
			const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
			const uint32 totalBones = skeleton->bonesCount();
			CAGE_ASSERT(temporary.size() == totalBones);
			Trs3 *local = (Trs3 *)CAGE_ALLOCA(sizeof(Trs3) * totalBones);

			// initialize from bind pose
			for (uint32 i = 0; i < totalBones; i++)
				local[i] = impl->baseMatrices[i];

			// blend layers
			for (const auto &layer : animations)
			{
				if (!layer.animation)
					continue;
				CAGE_ASSERT(valid(layer.coefficient));
				CAGE_ASSERT(layer.weight == saturate(layer.weight));
				CAGE_ASSERT(layer.mask.empty() || layer.mask.size() == totalBones);
				const SkeletalAnimationImpl *anim = (const SkeletalAnimationImpl *)layer.animation;
				SkeletalAnimationBlendingModeFlags flags = layer.blendingMode;
				if (any(flags & SkeletalAnimationBlendingModeFlags::Default))
					flags |= anim->blendingMode;
				Real coeff = layer.coefficient;
				if (any(flags & SkeletalAnimationBlendingModeFlags::Loop))
					coeff = coeff - floor(coeff);
				coeff = saturate(coeff);
				const auto additive = any(flags & SkeletalAnimationBlendingModeFlags::Additive);
				for (uint32 i = 0; i < totalBones; i++)
				{
					const Trs3 src = anim->evaluateBone(numeric_cast<uint16>(i), coeff, impl->baseMatrices[i]);
					const Real mask = (layer.mask.empty() ? 1 : layer.mask[i]) * layer.weight;
					if (additive)
						local[i] = additiveBlending(local[i], src, impl->baseMatrices[i], mask);
					else
						local[i] = overrideBlending(local[i], src, mask);
				}
			}

			// hierarchy solve
			for (uint32 i = 0; i < totalBones; i++)
			{
				const uint16 p = impl->boneParents[i];
				temporary[i] = convert(local[i]);
				if (p != m)
				{
					CAGE_ASSERT(p < i);
					temporary[i] = temporary[p] * temporary[i];
				}
				CAGE_ASSERT(temporary[i].valid());
			}
		}
	}

	void animateSkin(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, PointerRange<Mat4> output)
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
		const uint32 totalBones = skeleton->bonesCount();
		CAGE_ASSERT(output.size() == totalBones);
		animateImpl(skeleton, animations, output);
		for (uint32 i = 0; i < totalBones; i++)
		{
			output[i] = impl->globalInverse * output[i] * impl->invRestMatrices[i];
			CAGE_ASSERT(output[i].valid());
		}
	}

	void animateSkeleton(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, PointerRange<Mat4> output)
	{
		const SkeletonRigImpl *impl = (const SkeletonRigImpl *)skeleton;
		const uint32 totalBones = skeleton->bonesCount();
		CAGE_ASSERT(output.size() == totalBones);
		Mat4 *tmp = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * totalBones);
		animateImpl(skeleton, animations, { tmp, tmp + totalBones });
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

	void animateMesh(const SkeletonRig *skeleton, PointerRange<const SkeletalAnimationBlendingLayer> animations, Mesh *mesh)
	{
		const uint32 totalBones = skeleton->bonesCount();
		Mat4 *tmp = (Mat4 *)CAGE_ALLOCA(sizeof(Mat4) * totalBones);
		animateSkin(skeleton, animations, { tmp, tmp + totalBones });
		meshApplyAnimation(mesh, { tmp, tmp + totalBones });
	}
}
