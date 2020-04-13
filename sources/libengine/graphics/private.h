#include <cage-engine/graphics.h>

#ifdef CAGE_ASSERT_ENABLED
#define GCHL_ENABLE_CONTEXT_BINDING_CHECKS
#endif

struct GLFWmonitor;

namespace cage
{
	namespace graphicsPrivat
	{
		void openglContextInitializeGeneral(Window *w);

#ifdef GCHL_ENABLE_CONTEXT_BINDING_CHECKS

		uint32 contextTypeIndexInitializer();

		template<class T>
		uint32 contextTypeIndex()
		{
			static const uint32 index = contextTypeIndexInitializer();
			return index;
		}

		void contextSetCurrentObjectType(uint32 typeIndex, uint32 id);
		uint32 contextGetCurrentObjectType(uint32 typeIndex);

		template<class T>
		void setCurrentObject(uint32 id)
		{
			contextSetCurrentObjectType(contextTypeIndex<T>(), id);
		}

		template<class T>
		uint32 getCurrentObject()
		{
			return contextGetCurrentObjectType(contextTypeIndex<T>());
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

		string getMonitorId(GLFWmonitor *monitor);
		GLFWmonitor *getMonitorById(const string &id);
	}

	using namespace graphicsPrivat;
}
