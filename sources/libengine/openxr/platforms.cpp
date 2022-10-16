#include <cage-core/core.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include <Windows.h>
#pragma comment(lib, "opengl32") // wglGetCurrentDC
#endif // CAGE_SYSTEM_WINDOWS

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

namespace cage
{
	namespace
	{
		template<class T>
		void init(T &t, XrStructureType type)
		{
			detail::memset(&t, 0, sizeof(t));
			t.type = type;
		}
	}

	namespace privat
	{
		XrResult plaformInitSession(XrInstance instance, XrSystemId systemId, XrSession &session)
		{
			XrResult res = m;

			{
				PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
				res = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);
				if (res != XrResult::XR_SUCCESS)
					return res;

				XrGraphicsRequirementsOpenGLKHR reqs;
				init(reqs, XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR);
				res = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &reqs);
				if (res != XrResult::XR_SUCCESS)
					return res;
			}

#ifdef CAGE_SYSTEM_WINDOWS

			XrGraphicsBindingOpenGLWin32KHR binding;
			init(binding, XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR);
			binding.hDC = wglGetCurrentDC();
			binding.hGLRC = wglGetCurrentContext();

#else

			CAGE_THROW_CRITICAL(NotImplemented, "openxr currently works on windows only");

			char binding[100] = {}; // dummy structure to allow compiling
			//XrGraphicsBindingOpenGLXlibKHR binding;
			//init(binding, XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR);

			// todo some platform specific magic

#endif // CAGE_SYSTEM_WINDOWS

			XrSessionCreateInfo info;
			init(info, XR_TYPE_SESSION_CREATE_INFO);
			info.next = &binding;
			info.systemId = systemId;
			res = xrCreateSession(instance, &info, &session);

			return res;
		}
	}
}
