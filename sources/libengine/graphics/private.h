#include <cage-core/typeIndex.h>
#include <cage-engine/graphicsError.h>

#ifdef CAGE_ASSERT_ENABLED
#define GCHL_ENABLE_CONTEXT_BINDING_CHECKS
#endif

struct GLFWmonitor;

namespace cage
{
	namespace graphicsPrivat
	{
#ifdef GCHL_ENABLE_CONTEXT_BINDING_CHECKS

		void contextSetCurrentObjectType(uint32 typeIndex, uint32 id);
		uint32 contextGetCurrentObjectType(uint32 typeIndex);

		template<class T>
		void setCurrentObject(uint32 id)
		{
			contextSetCurrentObjectType(detail::typeIndex<T>(), id);
		}

		template<class T>
		uint32 getCurrentObject()
		{
			return contextGetCurrentObjectType(detail::typeIndex<T>());
		}

		void setCurrentContext(Window *context);
		Window *getCurrentContext();

#else

		template<class T>
		void setCurrentObject(uint32 id) {}

		template<class T>
		uint32 getCurrentObject() { return m; }

		inline void setCurrentContext(Window *context) {}
		inline Window *getCurrentContext() { return nullptr; }

#endif
	}

	using namespace graphicsPrivat;
}
