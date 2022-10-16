#include <cage-core/files.h> // pathExtractFilename, executableFullPathNoExe
#include <cage-engine/virtualReality.h>

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <cstring> // strcpy

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

		class VirtualRealityImpl : public VirtualReality
		{
		public:
			VirtualRealityImpl()
			{
				initInstance();
			}

			~VirtualRealityImpl()
			{
				if (instance)
				{
					xrDestroyInstance(instance);
					instance = XR_NULL_HANDLE;
				}
			}

			void check(XrResult result)
			{
				if (result == XrResult::XR_SUCCESS)
					return;

				if (instance)
				{
					char s[XR_MAX_RESULT_STRING_SIZE] = {};
					xrResultToString(instance, result, s);
					if (s[0])
						CAGE_LOG_THROW(s);
				}

				CAGE_THROW_ERROR(Exception, "openxr_error");
			}

			void initInstance()
			{
				const char *enabled_exts[1] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
				XrInstanceCreateInfo info;
				init(info, XR_TYPE_INSTANCE_CREATE_INFO);
				info.enabledExtensionCount = sizeof(enabled_exts) / sizeof(enabled_exts[0]);
				info.enabledExtensionNames = enabled_exts;
				info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
				std::strcpy(info.applicationInfo.applicationName, pathExtractFilename(detail::executableFullPathNoExe()).c_str());
				std::strcpy(info.applicationInfo.engineName, "Cage");

				check(xrCreateInstance(&info, &instance));

				// todo check(load_extension_function_pointers(instance));
			}

			static constexpr const XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
			static constexpr const XrViewConfigurationType view_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;


			XrInstance instance = XR_NULL_HANDLE;
			XrSystemId system_id = XR_NULL_SYSTEM_ID;
			XrSession session = XR_NULL_HANDLE;
			//XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl; // todo

			XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
			XrSpace play_space = XR_NULL_HANDLE;

			std::vector<XrViewConfigurationView> viewconfig_views;
			std::vector<XrCompositionLayerProjectionView> projection_views;
			std::vector<XrView> views;
			std::vector<XrSwapchain> swapchains, depth_swapchains;
			std::vector<std::vector<XrSwapchainImageOpenGLKHR>> images, depth_images;

		};
	}

	void VirtualReality::processEvents()
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		// todo
	}

	Holder<VirtualReality> newVirtualReality()
	{
		return systemMemory().createImpl<VirtualReality, VirtualRealityImpl>();
	}
}
