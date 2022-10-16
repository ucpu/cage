#include <cage-core/files.h> // pathExtractFilename, executableFullPathNoExe
#include <cage-core/config.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/opengl.h>

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <array>
#include <vector>
#include <cstring> // strcpy

// taking inspiration from https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/-/blob/master/main.c

namespace cage
{
	namespace privat
	{
		XrResult plaformInitSession(XrInstance instance, XrSystemId systemId, XrSession &session);
	}

	namespace
	{
		const ConfigBool confPrintApiLayers("cage/virtualReality/openxrLogApiLayers", true);
		const ConfigBool confPrintExtensions("cage/virtualReality/openxrLogExtensions", true);

		template<class T>
		void init(T &t, XrStructureType type)
		{
			detail::memset(&t, 0, sizeof(t));
			t.type = type;
		}

		class VirtualRealityImpl : public VirtualReality
		{
		public:
			VirtualRealityImpl(const VirtualRealityCreateConfig &config)
			{
				if (confPrintApiLayers)
					printApiLayers();
				if (confPrintExtensions)
					printExtensions();
				initInstance();
				initSystem();
				initViewsConfigurations();
				initSession();
				initReferenceSpace();
				initSwapchains();
				initViewsProjections();
				// todo init input
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

				CAGE_THROW_ERROR(SystemError, "openxr error", result);
			}

			void printApiLayers()
			{
				uint32_t count = 0;
				check(xrEnumerateApiLayerProperties(0, &count, NULL));

				std::vector<XrApiLayerProperties> props;
				props.resize(count);
				for (auto &it : props)
					init(it, XR_TYPE_API_LAYER_PROPERTIES);

				check(xrEnumerateApiLayerProperties(count, &count, props.data()));

				CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr api layers: ");
				for (const auto &it : props)
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "virtualReality", Stringizer() + "api layer: " + it.layerName + " " + it.layerVersion + " " + it.description);
			}

			void printExtensions()
			{
				uint32_t count = 0;
				check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr));

				std::vector<XrExtensionProperties> props;
				props.resize(count);
				for (auto &it : props)
					init(it, XR_TYPE_EXTENSION_PROPERTIES);

				check(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, props.data()));

				CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr extensions: ");
				for (const auto &it : props)
					CAGE_LOG_CONTINUE(SeverityEnum::Info, "virtualReality", Stringizer() + "extension: " + it.extensionName + " " + it.extensionVersion);
			}

			void initInstance()
			{
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
				}

				{
					XrInstanceProperties props;
					init(props, XR_TYPE_INSTANCE_PROPERTIES);
					check(xrGetInstanceProperties(instance, &props));
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr runtime name: " + props.runtimeName);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr runtime version: " + XR_VERSION_MAJOR(props.runtimeVersion) + "." + XR_VERSION_MINOR(props.runtimeVersion) + "." + XR_VERSION_PATCH(props.runtimeVersion));
				}

				// todo check(load_extension_function_pointers(instance));
			}

			void initSystem()
			{
				{
					XrSystemGetInfo info;
					init(info, XR_TYPE_SYSTEM_GET_INFO);
					info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
					check(xrGetSystem(instance, &info, &system_id));
				}

				{
					XrSystemProperties props;
					init(props, XR_TYPE_SYSTEM_PROPERTIES);
					check(xrGetSystemProperties(instance, system_id, &props));
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr system id: " + props.systemId);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "system name: " + props.systemName);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "vendor id: " + props.vendorId);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "max system resolution: " + props.graphicsProperties.maxSwapchainImageWidth + "x" + props.graphicsProperties.maxSwapchainImageHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "orientation tracking: " + (bool)props.trackingProperties.orientationTracking);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "position tracking: " + (bool)props.trackingProperties.positionTracking);
				}
			}

			void initViewsConfigurations()
			{
				{
					uint32_t view_count = 0;
					check(xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &view_count, NULL));
					if (view_count != 2)
						CAGE_THROW_ERROR(NotImplemented, "virtual reality requires two eyes");
					for (auto &it : viewconfig_views)
						init(it, XR_TYPE_VIEW_CONFIGURATION_VIEW);
					check(xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &view_count, viewconfig_views.data()));
				}

				{
					// assume that properties are the same for both eyes
					const auto &v = viewconfig_views[0];
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view resolution: " + v.recommendedImageRectWidth + "x" + v.recommendedImageRectHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "max view resolution: " + v.maxImageRectWidth + "x" + v.maxImageRectHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view samples: " + v.recommendedSwapchainSampleCount + ", max: " + v.maxSwapchainSampleCount);
				}
			}

			void initSession()
			{
				check(privat::plaformInitSession(instance, system_id, session));
			}

			void initReferenceSpace()
			{
				static constexpr const XrPosef identity_pose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0}, .position = {.x = 0, .y = 0, .z = 0} };
				XrReferenceSpaceCreateInfo info;
				init(info, XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
				info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
				info.poseInReferenceSpace = identity_pose;
				check(xrCreateReferenceSpace(session, &info, &play_space));
			}

			void createSwapchain(uint32 w, uint32 h, uint32 sampleCount, sint64 format, XrSwapchainUsageFlags flags, XrSwapchain &swapchain, std::vector<XrSwapchainImageOpenGLKHR> &images)
			{
				{
					XrSwapchainCreateInfo info;
					init(info, XR_TYPE_SWAPCHAIN_CREATE_INFO);
					info.usageFlags = flags;
					info.format = format;
					info.sampleCount = sampleCount;
					info.width = w;
					info.height = h;
					info.faceCount = 1;
					info.arraySize = 1;
					info.mipCount = 1;
					check(xrCreateSwapchain(session, &info, &swapchain));
				}

				{
					uint32 count = 0;
					check(xrEnumerateSwapchainImages(swapchain, 0, &count, NULL));
					images.resize(count);
					for (auto &it : images)
						init(it, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR);
					check(xrEnumerateSwapchainImages(swapchain, count, &count, (XrSwapchainImageBaseHeader *)images.data()));
				}
			}

			void initSwapchains()
			{
				uint32_t count = 0;
				check(xrEnumerateSwapchainFormats(session, 0, &count, NULL));
				std::vector<int64_t> swapchain_formats;
				swapchain_formats.resize(count);
				check(xrEnumerateSwapchainFormats(session, count, &count, swapchain_formats.data()));

				for (int i = 0; i < 2; i++)
				{
					const auto &v = viewconfig_views[i];
					createSwapchain(v.recommendedImageRectWidth, v.recommendedImageRectHeight, v.recommendedSwapchainSampleCount, GL_SRGB8_ALPHA8, XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT, color_swapchains[i], color_images[i]);
					createSwapchain(v.recommendedImageRectWidth, v.recommendedImageRectHeight, v.recommendedSwapchainSampleCount, GL_DEPTH_COMPONENT16, XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_swapchains[i], depth_images[i]);
				}
			}

			void initViewsProjections()
			{
				for (uint32 i = 0; i < 2; i++)
				{
					const auto &v = viewconfig_views[i];
					init(views[i], XR_TYPE_VIEW);

					auto &cv = color_views[i];
					init(cv, XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW);
					cv.subImage.swapchain = color_swapchains[i];
					cv.subImage.imageRect.extent.width = v.recommendedImageRectWidth;
					cv.subImage.imageRect.extent.height = v.recommendedImageRectHeight;

					auto &dv = depth_views[i];
					init(dv, XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR);
					dv.minDepth = 0;
					dv.maxDepth = 1;
					dv.subImage.swapchain = depth_swapchains[i];
					dv.subImage.imageRect.extent.width = v.recommendedImageRectWidth;
					dv.subImage.imageRect.extent.height = v.recommendedImageRectHeight;

					cv.next = &dv; // chain the depth to the color views
				}
			}

			XrInstance instance = XR_NULL_HANDLE;
			XrSystemId system_id = XR_NULL_SYSTEM_ID;
			XrSession session = XR_NULL_HANDLE;
			XrSpace play_space = XR_NULL_HANDLE;

			std::array<XrViewConfigurationView, 2> viewconfig_views = {};
			std::array<XrSwapchain, 2> color_swapchains, depth_swapchains;
			std::array<std::vector<XrSwapchainImageOpenGLKHR>, 2> color_images, depth_images;
			std::array<XrView, 2> views;
			std::array<XrCompositionLayerProjectionView, 2> color_views;
			std::array<XrCompositionLayerDepthInfoKHR, 2> depth_views;
		};
	}

	void VirtualReality::processEvents()
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		// todo
	}

	Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config)
	{
		return systemMemory().createImpl<VirtualReality, VirtualRealityImpl>(config);
	}
}
