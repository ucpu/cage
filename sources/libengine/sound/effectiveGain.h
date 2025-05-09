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
			// sort by priority
			std::sort(active.begin(), active.end(), [](const T a, const T b) -> bool { return std::pair(a->priority, a->effectiveGain) > std::pair(b->priority, b->effectiveGain); });
			for (uint32 i = ths->maxActiveSounds; i < active.size(); i++)
				Dereferencer(active[i])->effectiveGain = 0;

			/*
			{ // fade-out sounds close to limit
				const uint32 s = max(ths->maxActiveSounds * 70 / 100, 2u);
				const Real f = 1.0 / (ths->maxActiveSounds - s + 1);
				for (uint32 i = s; i < active.size(); i++)
					Dereferencer(active[i])->effectiveGain *= saturate(1 - (i - s + 1) * f);
			}
			*/

			// total gain reduction
			if (ths->maxGainThreshold != Real::Infinity())
			{
				Real sum;
				for (T a : active)
					sum += a->effectiveGain;
				if (sum > ths->maxGainThreshold)
				{
					const Real f = sqrt(ths->maxGainThreshold / sum);
					for (T a : active)
						a->effectiveGain *= f;
				}
			}

			// apply this gain
			for (T a : active)
				a->effectiveGain *= ths->gain;
		}
	}
}
