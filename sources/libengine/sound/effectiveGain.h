#include <algorithm>
#include <vector>

#include <cage-core/math.h>

namespace cage
{
	namespace
	{
		template<class T>
		struct Dereferencer : private Immovable
		{
			T *data = nullptr;
			CAGE_FORCE_INLINE Dereferencer(T *d) : data(d) {}
			CAGE_FORCE_INLINE Dereferencer(T &d) : data(&d) {}
			CAGE_FORCE_INLINE T *operator->() const { return data; }
		};

		template<class This, class K, class T = Dereferencer<std::remove_pointer_t<K>>>
		void updateEffectiveGains(const This *ths, std::vector<K> &active)
		{
			if (active.empty())
				return;

			// sort by priority
			std::sort(active.begin(), active.end(), [](const T a, const T b) -> bool { return std::pair(a->priority, a->effectiveGain) > std::pair(b->priority, b->effectiveGain); });

			// disable sounds over limit
			for (uint32 i = ths->maxSounds; i < active.size(); i++)
				Dereferencer(active[i])->effectiveGain = 0;
			const uint32 cnt = min((uintPtr)active.size(), (uintPtr)ths->maxSounds);
			const PointerRange<K> range = PointerRange<K>(active).subRange(0, cnt);

			// fade-out sounds close to limit
			if (ths->maxSounds > 5)
			{
				const uint32 s = max((uintPtr)ths->maxSounds * 70 / 100, (uintPtr)2);
				const Real f = 1.0 / (ths->maxSounds - s + 1);
				for (uint32 i = s; i < cnt; i++)
					Dereferencer(active[i])->effectiveGain *= saturate(1 - (i - s + 1) * f);
			}

			// max gain reduction
			if (ths->maxGain != Real::Infinity())
			{
				Real sum = 0;
				for (T a : range)
					sum += a->effectiveGain;
				sum = interpolate(sum, cnt, 0.5);
				if (sum > ths->maxGain)
				{
					const Real f = pow(ths->maxGain / sum, 0.7);
					for (T a : range)
						a->effectiveGain *= f;
				}
			}

			// apply this gain
			for (T a : range)
				a->effectiveGain *= ths->gain;
		}
	}
}
