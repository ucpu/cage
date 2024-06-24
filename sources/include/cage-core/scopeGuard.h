#ifndef guard_scopeGuard_h_gj4hgs1ev8gfzujsd
#define guard_scopeGuard_h_gj4hgs1ev8gfzujsd

#include <cage-core/core.h>

namespace cage
{
	template<class Callable>
	struct ScopeGuard : private Noncopyable
	{
		ScopeGuard() = default;
		[[nodiscard]] ScopeGuard(Callable &&callable) : callable(std::move(callable)) {}

		~ScopeGuard() noexcept
		{
			if (dismissed)
				return;
			try
			{
				callable();
			}
			catch (...)
			{
				detail::logCurrentCaughtException();
				detail::irrecoverableError("exception in ~ScopeGuard");
			}
		}

		void dismiss() { dismissed = true; }

	private:
		Callable callable = {};
		bool dismissed = false;
	};
}

#endif // guard_scopeGuard_h_gj4hgs1ev8gfzujsd
