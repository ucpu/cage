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
			for (uint32 i = ths->maxSounds; i < active.size(); i++)
				Dereferencer(active[i])->effectiveGain = 0;

			/*
			{ // fade-out sounds close to limit
				const uint32 s = max(ths->maxSounds * 70 / 100, 2u);
				const Real f = 1.0 / (ths->maxSounds - s + 1);
				for (uint32 i = s; i < active.size(); i++)
					Dereferencer(active[i])->effectiveGain *= saturate(1 - (i - s + 1) * f);
			}
			*/

			// max gain reduction
			const uint32 cnt = min((uintPtr)active.size(), (uintPtr)ths->maxSounds);
			if (cnt > ths->maxGain)
			{
				const Real f = pow(ths->maxGain / cnt, 0.9);
				for (T a : active)
					a->effectiveGain *= f;
			}

			// apply this gain
			for (T a : active)
				a->effectiveGain *= ths->gain;
		}
	}
}
