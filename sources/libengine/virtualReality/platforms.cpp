#ifdef CAGE_SYSTEM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <Windows.h>
	#pragma comment(lib, "opengl32") // wglGetCurrentDC
	#define XR_USE_PLATFORM_WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif // CAGE_SYSTEM_WINDOWS

#ifdef CAGE_SYSTEM_LINUX
	#if defined(_GLFW_X11)
		#define XR_USE_PLATFORM_XLIB
		#define GLFW_EXPOSE_NATIVE_X11
	#endif
	#if defined(_GLFW_WAYLAND)
		#define XR_USE_PLATFORM_WAYLAND
		#define GLFW_EXPOSE_NATIVE_WAYLAND
	#endif
#endif // CAGE_SYSTEM_LINUX

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <cage-core/core.h>

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
				PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
				res = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);
				if (!XR_SUCCEEDED(res))
					return res;

				XrGraphicsRequirementsOpenGLKHR reqs;
				init(reqs, XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR);
				res = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &reqs);
				if (!XR_SUCCEEDED(res))
					return res;
			}

#ifdef CAGE_SYSTEM_WINDOWS

			CAGE_ASSERT(glfwGetPlatform() == GLFW_PLATFORM_WIN32);
			XrGraphicsBindingOpenGLWin32KHR binding;
			init(binding, XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR);
			binding.hDC = wglGetCurrentDC();
			binding.hGLRC = wglGetCurrentContext();

#elif defined(XR_USE_PLATFORM_XLIB)

			if (glfwGetPlatform() != GLFW_PLATFORM_X11)
				CAGE_THROW_ERROR(Exception, "cannot initialize OpenXR for x11, glfw platform differs");
			XrGraphicsBindingOpenGLXlibKHR binding;
			init(binding, XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR);
			binding.xDisplay = glfwGetX11Display();
			// binding.visualid
			// binding.glxFBConfig
			binding.glxDrawable = glXGetCurrentDrawable();
			binding.glxContext = glXGetCurrentContext();

#elif defined(XR_USE_PLATFORM_WAYLAND)

			if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND)
				CAGE_THROW_ERROR(Exception, "cannot initialize OpenXR for wayland, glfw platform differs");
			XrGraphicsBindingOpenGLWaylandKHR binding;
			init(binding, XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR);
			binding.Display = glfwGetWaylandDisplay();

#else

			CAGE_THROW_ERROR(Exception, "unsupported platform for OpenXR in cage");
			char binding[100] = {}; // dummy structure to allow compiling

#endif

			XrSessionCreateInfo info;
			init(info, XR_TYPE_SESSION_CREATE_INFO);
			info.next = &binding;
			info.systemId = systemId;
			res = xrCreateSession(instance, &info, &session);

			return res;
		}
	}
}
