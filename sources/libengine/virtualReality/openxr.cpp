#include <cage-core/files.h> // pathExtractFilename, executableFullPathNoExe
#include <cage-core/config.h>
#include <cage-core/profiling.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/texture.h>
#include <cage-engine/opengl.h>

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#include <array>
#include <tuple>
#include <vector>
#include <map>
#include <cstring> // strcpy
#include <cmath> // tan
#include <atomic>

#ifndef XR_APILAYER_LUNARG_core_validation
#define XR_APILAYER_LUNARG_core_validation "XR_APILAYER_LUNARG_core_validation"
#endif

// taking inspiration from https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/-/blob/master/main.c

namespace cage
{
	namespace privat
	{
		XrResult plaformInitSession(XrInstance instance, XrSystemId systemId, XrSession &session);
		Holder<Texture> createTextureForOpenXr(uint32 id, uint32 internalFormat, Vec2i resolution);
		void loadControllerBindings(XrInstance instance, const char *sideName, std::map<String, std::vector<XrActionSuggestedBinding>> &suggestions, PointerRange<const XrAction> axesActions, PointerRange<const XrAction> butsActions);
		void controllerBindingsCheckUnused();
	}

	namespace
	{
		const ConfigBool confPrintApiLayers("cage/virtualReality/printApiLayers", false);
		const ConfigBool confPrintExtensions("cage/virtualReality/printExtensions", false);
		const ConfigBool confEnableValidation("cage/virtualReality/validationLayer", CAGE_DEBUG_BOOL);
		const ConfigBool confEnableDebugUtils("cage/virtualReality/debugUtils", CAGE_DEBUG_BOOL);

		constexpr const XrPosef IdentityPose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0}, .position = {.x = 0, .y = 0, .z = 0} };
		constexpr const Transform InvalidTransform = Transform(Vec3(), Quat(), 0);
		constexpr const XrSpaceLocationFlags ValidMask = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
		constexpr const XrSpaceLocationFlags TrackingMask = XR_SPACE_LOCATION_POSITION_TRACKED_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;

		template<class T>
		CAGE_FORCE_INLINE void init(T &t, XrStructureType type)
		{
			detail::memset(&t, 0, sizeof(t));
			t.type = type;
		}

		XrBool32 XRAPI_PTR debugUtilsCallback(XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT *msg, void *user_data)
		{
			const class VirtualRealityImpl *impl = (VirtualRealityImpl *)user_data;
			SeverityEnum cageSeverity = SeverityEnum::Critical;
			switch (severity)
			{
			case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				cageSeverity = SeverityEnum::Error;
				break;
			case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				cageSeverity = SeverityEnum::Warning;
				break;
			case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				cageSeverity = SeverityEnum::Info;
				break;
			case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				cageSeverity = SeverityEnum::Note;
				break;
			}
			CAGE_LOG(cageSeverity, "openxr", msg->message);
			return XR_FALSE;
		}

		CAGE_FORCE_INLINE Transform poseToTranform(const XrPosef &pose)
		{
			static_assert(sizeof(Vec3) == sizeof(XrVector3f));
			static_assert(sizeof(Quat) == sizeof(XrQuaternionf));
			Transform res;
			res.position = *(Vec3 *)&pose.position;
			res.orientation = *(Quat *)&pose.orientation;
			return res;
		}

		CAGE_FORCE_INLINE Mat4 fovToProjection(const XrFovf &fov, float nearZ, float farZ)
		{
			const float tanAngleLeft = std::tan(fov.angleLeft);
			const float tanAngleRight = std::tan(fov.angleRight);
			const float tanAngleDown = std::tan(fov.angleDown);
			const float tanAngleUp = std::tan(fov.angleUp);
			const float tanAngleWidth = tanAngleRight - tanAngleLeft;
			const float tanAngleHeight = (tanAngleUp - tanAngleDown);

			Mat4 result;
			result.data[0] = 2 / tanAngleWidth;
			result.data[4] = 0;
			result.data[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
			result.data[12] = 0;
			result.data[1] = 0;
			result.data[5] = 2 / tanAngleHeight;
			result.data[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
			result.data[13] = 0;
			result.data[2] = 0;
			result.data[6] = 0;
			result.data[10] = -(farZ + nearZ) / (farZ - nearZ);
			result.data[14] = -(farZ * (nearZ + nearZ)) / (farZ - nearZ);
			result.data[3] = 0;
			result.data[7] = 0;
			result.data[11] = -1;
			result.data[15] = 0;
			return result;
		}

		CAGE_FORCE_INLINE bool valueChange(bool &prev, bool next)
		{
			const bool res = prev != next;
			prev = next;
			return res;
		}

		class VirtualRealityControllerImpl : public VirtualRealityController
		{
		public:
			struct Pose
			{
				XrAction action = 0;
				XrSpace space = 0;
				Transform transform = InvalidTransform;
			} aim, grip;

			std::array<Real, 6> axes = {};
			std::array<XrAction, 6> axesActions = {};
			std::array<bool, 10> buts = {};
			std::array<XrAction, 10> butsActions = {};

			bool tracking = false;
		};

		class VirtualRealityImpl : public VirtualReality
		{
		public:
			VirtualRealityImpl(const VirtualRealityCreateConfig &config)
			{
				CAGE_LOG(SeverityEnum::Info, "virtualReality", "initializing openxr");
				printApiLayers();
				printExtensions();
				initHandles();
				initGraphics();
				initInputs();
			}

			~VirtualRealityImpl()
			{
				if (instance)
				{
					xrDestroyInstance(instance);
					instance = XR_NULL_HANDLE;
				}
			}

			CAGE_FORCE_INLINE void check(XrResult result) const
			{
				if (XR_SUCCEEDED(result))
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
				uint32 count = 0;
				check(xrEnumerateApiLayerProperties(0, &count, nullptr));

				std::vector<XrApiLayerProperties> props;
				props.resize(count);
				for (auto &it : props)
					init(it, XR_TYPE_API_LAYER_PROPERTIES);

				check(xrEnumerateApiLayerProperties(count, &count, props.data()));

				if (confPrintApiLayers)
				{
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr api layers: ");
					for (const auto &it : props)
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "virtualReality", Stringizer() + "api layer: " + it.layerName + ", version: " + it.layerVersion);
				}

				for (const auto &it : props)
				{
					if (String(it.layerName) == XR_APILAYER_LUNARG_core_validation)
						haveApilayerCoreValidation = true;
				}
			}

			void printExtensions()
			{
				uint32 count = 0;
				check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr));

				std::vector<XrExtensionProperties> props;
				props.resize(count);
				for (auto &it : props)
					init(it, XR_TYPE_EXTENSION_PROPERTIES);

				check(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, props.data()));

				if (confPrintExtensions)
				{
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr extensions: ");
					for (const auto &it : props)
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "virtualReality", Stringizer() + "extension: " + it.extensionName + ", version: " + it.extensionVersion);
				}

				for (const auto &it : props)
				{
					if (String(it.extensionName) == XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)
						haveKhrOpenglEnable = true;
					if (String(it.extensionName) == XR_EXT_DEBUG_UTILS_EXTENSION_NAME)
						haveExtDebugUtils = true;
					if (String(it.extensionName) == XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME)
						haveFbDisplayRefreshRate = true;
				}
			}

			void initHandles()
			{
				{
					std::vector<const char *> exts, layers;

					if (haveKhrOpenglEnable)
						exts.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

					if (confEnableDebugUtils)
					{
						if (haveExtDebugUtils)
						{
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "enabling extension debug utils");
							exts.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
						}
						else
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "requested extension debug utils is not available - skipping");
					}
					else
						haveExtDebugUtils = false;

					if (haveFbDisplayRefreshRate)
					{
						CAGE_LOG(SeverityEnum::Info, "virtualReality", "enabling extension display refresh rate");
						exts.push_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
					}

					if (confEnableValidation)
					{
						if (haveApilayerCoreValidation)
						{
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "enabling api validation layer");
							layers.push_back(XR_APILAYER_LUNARG_core_validation);
						}
						else
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "requested api validation layer is not available - skipping");
					}
					else
						haveApilayerCoreValidation = false;

					XrInstanceCreateInfo info;
					init(info, XR_TYPE_INSTANCE_CREATE_INFO);
					info.enabledExtensionCount = exts.size();
					info.enabledExtensionNames = exts.data();
					info.enabledApiLayerCount = layers.size();
					info.enabledApiLayerNames = layers.data();
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

				if (!haveKhrOpenglEnable)
					CAGE_THROW_ERROR(Exception, "missing required OpenXR extension: XR_KHR_OPENGL_ENABLE")

				if (haveExtDebugUtils)
				{
					PFN_xrCreateDebugUtilsMessengerEXT createMessenger = nullptr;
					check(xrGetInstanceProcAddr(instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)&createMessenger));

					XrDebugUtilsMessengerCreateInfoEXT info;
					init(info, XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
					info.messageTypes =
						XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
					info.messageSeverities =
						XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
					info.userCallback = &debugUtilsCallback;
					info.userData = this;
					XrDebugUtilsMessengerEXT debug = {};
					check(createMessenger(instance, &info, &debug));
				}

				if (haveFbDisplayRefreshRate)
					check(xrGetInstanceProcAddr(instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction *)&xrGetDisplayRefreshRateFB));

				{
					XrSystemGetInfo info;
					init(info, XR_TYPE_SYSTEM_GET_INFO);
					info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
					check(xrGetSystem(instance, &info, &systemId));
				}

				{
					XrSystemProperties props;
					init(props, XR_TYPE_SYSTEM_PROPERTIES);
					check(xrGetSystemProperties(instance, systemId, &props));
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "openxr system id: " + props.systemId);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "system name: " + props.systemName);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "vendor id: " + props.vendorId);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "max system resolution: " + props.graphicsProperties.maxSwapchainImageWidth + "x" + props.graphicsProperties.maxSwapchainImageHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "orientation tracking available: " + (bool)props.trackingProperties.orientationTracking);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "position tracking available: " + (bool)props.trackingProperties.positionTracking);
				}

				check(privat::plaformInitSession(instance, systemId, session));

				{
					XrReferenceSpaceCreateInfo info;
					init(info, XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
					info.poseInReferenceSpace = IdentityPose;
					info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
					check(xrCreateReferenceSpace(session, &info, &localSpace));
					info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
					check(xrCreateReferenceSpace(session, &info, &viewSpace));
				}
			}

			void createSwapchain(sint64 format, XrSwapchainUsageFlags flags, XrSwapchain &swapchain, std::vector<XrSwapchainImageOpenGLKHR> &images, std::vector<Holder<Texture>> &textures)
			{
				const auto &v = viewConfigs[0];

				{
					XrSwapchainCreateInfo info;
					init(info, XR_TYPE_SWAPCHAIN_CREATE_INFO);
					info.usageFlags = flags;
					info.format = format;
					info.width = v.recommendedImageRectWidth;
					info.height = v.recommendedImageRectHeight;
					info.sampleCount = 1;
					info.faceCount = 1;
					info.arraySize = 1;
					info.mipCount = 1;
					check(xrCreateSwapchain(session, &info, &swapchain));
				}

				{
					uint32 count = 0;
					check(xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr));
					images.resize(count);
					for (auto &it : images)
						init(it, XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR);
					check(xrEnumerateSwapchainImages(swapchain, count, &count, (XrSwapchainImageBaseHeader *)images.data()));
				}

				{
					textures.reserve(images.size());
					for (auto it : images)
						textures.push_back(privat::createTextureForOpenXr(it.image, numeric_cast<uint32>(format), Vec2i(v.recommendedImageRectWidth, v.recommendedImageRectHeight)));
				}
			}

			void initGraphics()
			{
				{
					uint32 count = 0;
					check(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &count, nullptr));
					if (count == 0)
						CAGE_THROW_ERROR(Exception, "virtual reality has no views");
					viewConfigs.resize(count);
					for (auto &it : viewConfigs)
						init(it, XR_TYPE_VIEW_CONFIGURATION_VIEW);
					check(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewConfigs.size(), &count, viewConfigs.data()));
					CAGE_ASSERT(count == viewConfigs.size());
				}
				const auto &v = viewConfigs[0];

				{
					// log properties for the first view
					CAGE_LOG_DEBUG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view resolution: " + v.recommendedImageRectWidth + "x" + v.recommendedImageRectHeight);
					CAGE_LOG_DEBUG(SeverityEnum::Info, "virtualReality", Stringizer() + "max view resolution: " + v.maxImageRectWidth + "x" + v.maxImageRectHeight);
					CAGE_LOG_DEBUG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view samples: " + v.recommendedSwapchainSampleCount + ", max: " + v.maxSwapchainSampleCount);
				}

				colorSwapchains.resize(viewConfigs.size());
				colorImages.resize(viewConfigs.size());
				colorTextures.resize(viewConfigs.size());
				colorViews.resize(viewConfigs.size());

				{
					uint32 count = 0;
					check(xrEnumerateSwapchainFormats(session, 0, &count, nullptr));
					std::vector<int64_t> formats;
					formats.resize(count);
					check(xrEnumerateSwapchainFormats(session, count, &count, formats.data()));

					const uint32 selectedFormat = [&]() {
						for (uint32 f : formats)
						{
							switch (f)
							{
							case GL_SRGB8_ALPHA8:
							case GL_SRGB8:
								return f;
							}
						}
						CAGE_THROW_ERROR(Exception, "no supported swapchain format");
					}();

					uint32 nameIndex = 0;
					for (uint32 i = 0; i < viewConfigs.size(); i++)
					{
						createSwapchain(selectedFormat, XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT, colorSwapchains[i], colorImages[i], colorTextures[i]);
						for (Holder<Texture> &it : colorTextures[i])
							it->setDebugName(Stringizer() + "openxr color " + nameIndex++);
					}
				}

				for (uint32 i = 0; i < viewConfigs.size(); i++)
				{
					auto &cv = colorViews[i];
					init(cv, XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW);
					cv.subImage.swapchain = colorSwapchains[i];
					cv.subImage.imageRect.extent.width = v.recommendedImageRectWidth;
					cv.subImage.imageRect.extent.height = v.recommendedImageRectHeight;
				}
			}

			void initInputs()
			{
				{
					XrActionSetCreateInfo actionsetInfo;
					init(actionsetInfo, XR_TYPE_ACTION_SET_CREATE_INFO);
					std::strcpy(actionsetInfo.actionSetName, "generic_actionset");
					std::strcpy(actionsetInfo.localizedActionSetName, "generic_actionset");
					check(xrCreateActionSet(instance, &actionsetInfo, &actionSet));
				}

				std::map<String, std::vector<XrActionSuggestedBinding>> suggestions;

				for (uint32 side = 0; side < 2; side++)
				{
					VirtualRealityControllerImpl &cntrl = controllers[side];
					static constexpr const char *sideNames[2] = { "left", "right" };
					const char *sideName = sideNames[side];

					for (uint32 i = 0; i < cntrl.axesActions.size(); i++)
					{
						static constexpr const char *axesNames[] = { "thumbstick_x", "thumbstick_y", "trackpad_x", "trackpad_y", "trigger_value", "squeeze_value" };
						static_assert(sizeof(axesNames) / sizeof(axesNames[0]) == decltype(cntrl.axesActions){}.size());
						XrActionCreateInfo info;
						init(info, XR_TYPE_ACTION_CREATE_INFO);
						info.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
						const String n = Stringizer() + sideName + "_" + axesNames[i];
						std::strcpy(info.actionName, n.c_str());
						std::strcpy(info.localizedActionName, n.c_str());
						check(xrCreateAction(actionSet, &info, &cntrl.axesActions[i]));
					}

					for (uint32 i = 0; i < cntrl.butsActions.size(); i++)
					{
						static constexpr const char *butsNames[] = { "a", "b", "x", "y", "thumbstick", "trackpad", "select", "menu", "trigger_click", "squeeze_click" };
						static_assert(sizeof(butsNames) / sizeof(butsNames[0]) == decltype(cntrl.butsActions){}.size());
						XrActionCreateInfo info;
						init(info, XR_TYPE_ACTION_CREATE_INFO);
						info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
						const String n = Stringizer() + sideName + "_" + butsNames[i];
						std::strcpy(info.actionName, n.c_str());
						std::strcpy(info.localizedActionName, n.c_str());
						check(xrCreateAction(actionSet, &info, &cntrl.butsActions[i]));
					}

					privat::loadControllerBindings(instance, sideName, suggestions, cntrl.axesActions, cntrl.butsActions);

					std::array<XrPath, 2> pathPoses; // aim, grip
					check(xrStringToPath(instance, (Stringizer() + "/user/hand/" + sideName + "/input/aim/pose").value.c_str(), &pathPoses[0]));
					check(xrStringToPath(instance, (Stringizer() + "/user/hand/" + sideName + "/input/grip/pose").value.c_str(), &pathPoses[1]));

					for (uint32 i = 0; i < 2; i++)
					{
						VirtualRealityControllerImpl::Pose &p = *std::array{ &cntrl.aim, &cntrl.grip }[i];
						static constexpr const char *ns[2] = { "aim", "grip" };
						const String n = Stringizer() + sideName + "_" + ns[i] + "_pose";

						{
							XrActionCreateInfo info;
							init(info, XR_TYPE_ACTION_CREATE_INFO);
							info.actionType = XR_ACTION_TYPE_POSE_INPUT;
							std::strcpy(info.actionName, n.c_str());
							std::strcpy(info.localizedActionName, n.c_str());
							check(xrCreateAction(actionSet, &info, &p.action));
							for (auto &it : suggestions)
								it.second.push_back(XrActionSuggestedBinding{ p.action, pathPoses[i] });
						}
						{
							XrActionSpaceCreateInfo info;
							init(info, XR_TYPE_ACTION_SPACE_CREATE_INFO);
							info.poseInActionSpace = IdentityPose;
							info.action = p.action;
							check(xrCreateActionSpace(session, &info, &p.space));
						}
					}
				}

				privat::controllerBindingsCheckUnused();

				for (const auto &it : suggestions)
				{
					XrPath pathProfile;
					check(xrStringToPath(instance, it.first.c_str(), &pathProfile));
					XrInteractionProfileSuggestedBinding info;
					init(info, XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING);
					info.interactionProfile = pathProfile;
					info.countSuggestedBindings = it.second.size();
					info.suggestedBindings = it.second.data();
					check(xrSuggestInteractionProfileBindings(instance, &info));
				}

				{
					XrSessionActionSetsAttachInfo info;
					init(info, XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO);
					info.countActionSets = 1;
					info.actionSets = &actionSet;
					check(xrAttachSessionActionSets(session, &info));
				}
			}

			void updateInputs()
			{
				{
					XrActiveActionSet active;
					active.actionSet = actionSet;
					active.subactionPath = XR_NULL_PATH;
					XrActionsSyncInfo info;
					init(info, XR_TYPE_ACTIONS_SYNC_INFO);
					info.countActiveActionSets = 1;
					info.activeActionSets = &active;
					check(xrSyncActions(session, &info));
				}

				const XrTime time = syncTime;
				if (time == 0)
					return;

				std::vector<GenericInput> eventsQueue;

				{
					XrSpaceLocation location;
					init(location, XR_TYPE_SPACE_LOCATION);
					check(xrLocateSpace(viewSpace, localSpace, time, &location));
					headTransform = poseToTranform(location.pose);
					if (valueChange(tracking, (location.locationFlags & TrackingMask) == TrackingMask))
						eventsQueue.push_back(GenericInput{ InputHeadsetState{ this }, tracking ? InputClassEnum::HeadsetConnected : InputClassEnum::HeadsetDisconnected });
					eventsQueue.push_back(GenericInput{ InputHeadsetPose{ headTransform, this }, InputClassEnum::HeadsetPose });
				}

				for (VirtualRealityControllerImpl &cntrl : controllers)
				{
					bool nextTracking = true;

					for (VirtualRealityControllerImpl::Pose *it : { &cntrl.aim, &cntrl.grip })
					{
						it->transform = InvalidTransform;
						XrActionStatePose state;
						init(state, XR_TYPE_ACTION_STATE_POSE);
						XrActionStateGetInfo info;
						init(info, XR_TYPE_ACTION_STATE_GET_INFO);
						info.action = it->action;
						check(xrGetActionStatePose(session, &info, &state));
						XrSpaceLocation location;
						init(location, XR_TYPE_SPACE_LOCATION);
						check(xrLocateSpace(it->space, localSpace, time, &location));
						it->transform = poseToTranform(location.pose);
						nextTracking &= (location.locationFlags & ValidMask) == ValidMask;
					}

					if (valueChange(cntrl.tracking, nextTracking))
						eventsQueue.push_back(GenericInput{ InputControllerState{ &cntrl }, nextTracking ? InputClassEnum::ControllerConnected : InputClassEnum::ControllerDisconnected });
					eventsQueue.push_back(GenericInput{ InputControllerPose{ cntrl.grip.transform, &cntrl }, InputClassEnum::ControllerPose });

					for (uint32 i = 0; i < cntrl.axes.size(); i++)
					{
						if (cntrl.axesActions[i] == 0)
							continue;
						XrActionStateFloat state;
						init(state, XR_TYPE_ACTION_STATE_FLOAT);
						XrActionStateGetInfo info;
						init(info, XR_TYPE_ACTION_STATE_GET_INFO);
						info.action = cntrl.axesActions[i];
						check(xrGetActionStateFloat(session, &info, &state));
						cntrl.axes[i] = state.currentState;
						if (state.changedSinceLastSync)
							eventsQueue.push_back(GenericInput{ InputControllerAxis{ &cntrl, i, state.currentState }, InputClassEnum::ControllerAxis });
					}

					for (uint32 i = 0; i < cntrl.buts.size(); i++)
					{
						if (cntrl.butsActions[i] == 0)
							continue;
						XrActionStateBoolean state;
						init(state, XR_TYPE_ACTION_STATE_BOOLEAN);
						XrActionStateGetInfo info;
						init(info, XR_TYPE_ACTION_STATE_GET_INFO);
						info.action = cntrl.butsActions[i];
						check(xrGetActionStateBoolean(session, &info, &state));
						cntrl.buts[i] = state.currentState;
						if (state.changedSinceLastSync)
							eventsQueue.push_back(GenericInput{ InputControllerKey{ &cntrl, i }, state.currentState ? InputClassEnum::ControllerPress : InputClassEnum::ControllerRelease });
					}
				}

				for (GenericInput &e : eventsQueue)
					events.dispatch(e);
			}

			void handleEvent(const XrEventDataBuffer &event)
			{
				switch (event.type)
				{
				case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
					stopping = true;
					break;
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
				{
					const XrEventDataSessionStateChanged &ev = (const XrEventDataSessionStateChanged &)event;
					switch (ev.state)
					{
					case XR_SESSION_STATE_IDLE:
					case XR_SESSION_STATE_UNKNOWN:
						break;
					case XR_SESSION_STATE_FOCUSED:
					case XR_SESSION_STATE_SYNCHRONIZED:
					case XR_SESSION_STATE_VISIBLE:
						break;
					case XR_SESSION_STATE_READY:
					{
						if (!sessionRunning)
						{
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "beginning openxr session");
							XrSessionBeginInfo info;
							init(info, XR_TYPE_SESSION_BEGIN_INFO);
							info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
							check(xrBeginSession(session, &info));
							sessionRunning = true;
						}
					} break;
					case XR_SESSION_STATE_STOPPING:
					{
						if (sessionRunning)
						{
							CAGE_LOG(SeverityEnum::Info, "virtualReality", "ending openxr session");
							check(xrEndSession(session));
							sessionRunning = false;
						}
					} break;
					case XR_SESSION_STATE_LOSS_PENDING:
					case XR_SESSION_STATE_EXITING:
						stopping = true;
						break;
					case XR_SESSION_STATE_MAX_ENUM:
						break;
					}
				} break;
				case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
					break;
				default:
					break;
				}
			}

			void pollEvents()
			{
				while (true)
				{
					XrEventDataBuffer event;
					init(event, XR_TYPE_EVENT_DATA_BUFFER);
					const XrResult res = xrPollEvent(instance, &event);
					if (res == XrResult::XR_SUCCESS)
						handleEvent(event);
					else if (res == XrResult::XR_EVENT_UNAVAILABLE)
						break;
					else
						check(res);
				}
			}

			Texture *acquireTexture(XrSwapchain swapchain, std::vector<Holder<Texture>> &textures)
			{
				uint32 acquiredIndex = 0;
				check(xrAcquireSwapchainImage(swapchain, nullptr, &acquiredIndex));

				XrSwapchainImageWaitInfo wait;
				init(wait, XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO);
				wait.timeout = XR_INFINITE_DURATION;
				check(xrWaitSwapchainImage(swapchain, &wait));

				return +textures[acquiredIndex];
			}

			bool haveKhrOpenglEnable = false; // XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
			bool haveExtDebugUtils = false; // XR_EXT_DEBUG_UTILS_EXTENSION_NAME
			bool haveFbDisplayRefreshRate = false; // XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME
			bool haveApilayerCoreValidation = false; // XR_APILAYER_LUNARG_core_validation

			XrInstance instance = XR_NULL_HANDLE;
			XrSystemId systemId = XR_NULL_SYSTEM_ID;
			XrSession session = XR_NULL_HANDLE;
			XrSpace localSpace = XR_NULL_HANDLE;
			XrSpace viewSpace = XR_NULL_HANDLE;

			std::vector<XrViewConfigurationView> viewConfigs = {};
			std::vector<XrSwapchain> colorSwapchains;
			std::vector<std::vector<XrSwapchainImageOpenGLKHR>> colorImages;
			std::vector<std::vector<Holder<Texture>>> colorTextures;
			std::vector<XrCompositionLayerProjectionView> colorViews;

			std::array<VirtualRealityControllerImpl, 2> controllers;
			XrActionSet actionSet;
			std::atomic<XrTime> syncTime = 0;

			bool stopping = false;
			bool sessionRunning = false;

			Transform headTransform = InvalidTransform;
			bool tracking = false;
			XrTime firstFrameTime = m;

			PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB = nullptr;
		};

		class VirtualRealityGraphicsFrameImpl : public VirtualRealityGraphicsFrame
		{
		public:
			VirtualRealityImpl *const impl = nullptr;
			XrFrameState frameState = {};
			std::vector<XrView> views;
			std::vector<VirtualRealityCamera> cams;
			Transform headTransform = InvalidTransform;
			bool rendering = false;

			VirtualRealityGraphicsFrameImpl(VirtualRealityImpl *impl) : impl(impl)
			{
				{
					init(frameState, XR_TYPE_FRAME_STATE);
					const XrResult res = xrWaitFrame(impl->session, nullptr, &frameState);
					switch (res)
					{
					case XR_SESSION_LOSS_PENDING:
					case XR_ERROR_INSTANCE_LOST:
					case XR_ERROR_SESSION_LOST:
					case XR_ERROR_SESSION_NOT_RUNNING:
						return;
					default:
						break;
					}
					check(res);
					impl->syncTime = frameState.predictedDisplayTime;
					if (impl->firstFrameTime == m)
						impl->firstFrameTime = frameState.predictedDisplayTime;
				}

				{
					XrSpaceLocation location;
					init(location, XR_TYPE_SPACE_LOCATION);
					check(xrLocateSpace(impl->viewSpace, impl->localSpace, frameState.predictedDisplayTime, &location));
					headTransform = poseToTranform(location.pose);
				}

				rendering = true;
				if (!frameState.shouldRender)
					return;

				views.resize(impl->viewConfigs.size());
				for (auto &it : views)
					init(it, XR_TYPE_VIEW);

				{
					XrViewLocateInfo viewLocateInfo;
					init(viewLocateInfo, XR_TYPE_VIEW_LOCATE_INFO);
					viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					viewLocateInfo.displayTime = frameState.predictedDisplayTime;
					viewLocateInfo.space = impl->localSpace;
					XrViewState viewState;
					init(viewState, XR_TYPE_VIEW_STATE);
					uint32 count = 0;
					check(xrLocateViews(impl->session, &viewLocateInfo, &viewState, views.size(), &count, views.data()));
					CAGE_ASSERT(count == views.size());
				}

				cams.resize(views.size());
				for (uint32 i = 0; i < cams.size(); i++)
				{
					cams[i].transform = poseToTranform(views[i].pose); // there is currently 1 projection per camera
					cams[i].resolution = Vec2i(impl->viewConfigs[i].recommendedImageRectWidth, impl->viewConfigs[i].recommendedImageRectHeight);
					cams[i].primary = true; // all cameras are primary for now
				}

				cameras = cams;
			}

			void updateProjections()
			{
				for (uint32 i = 0; i < cams.size(); i++)
				{
					cams[i].projection = fovToProjection(views[i].fov, cams[i].nearPlane.value, cams[i].farPlane.value);
					cams[i].verticalFov = Rads(views[i].fov.angleUp - views[i].fov.angleDown);
				}
			}

			void begin()
			{
				if (!rendering)
					return;

				check(xrBeginFrame(impl->session, nullptr));
			}

			void acquire()
			{
				if (!rendering)
					return;

				if (frameState.shouldRender)
					for (uint32 i = 0; i < cams.size(); i++)
						cams[i].colorTexture = impl->acquireTexture(impl->colorSwapchains[i], impl->colorTextures[i]);
			}

			void commit()
			{
				if (!rendering)
					return;

				if (frameState.shouldRender)
					for (uint32 i = 0; i < views.size(); i++)
						check(xrReleaseSwapchainImage(impl->colorSwapchains[i], nullptr));

				for (uint32 i = 0; i < views.size(); i++)
				{
					impl->colorViews[i].pose = views[i].pose;
					impl->colorViews[i].fov = views[i].fov;
				}

				XrCompositionLayerProjection projectionLayer;
				init(projectionLayer, XR_TYPE_COMPOSITION_LAYER_PROJECTION);
				projectionLayer.space = impl->localSpace;
				projectionLayer.viewCount = impl->colorViews.size();
				projectionLayer.views = impl->colorViews.data();
				projectionLayer.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
				std::array<const XrCompositionLayerBaseHeader *, 1> submittedLayers = { (XrCompositionLayerBaseHeader *)&projectionLayer };

				XrFrameEndInfo frameEndInfo;
				init(frameEndInfo, XR_TYPE_FRAME_END_INFO);
				frameEndInfo.layerCount = frameState.shouldRender ? submittedLayers.size() : 0;
				frameEndInfo.layers = submittedLayers.data();
				frameEndInfo.displayTime = frameState.predictedDisplayTime;
				frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
				check(xrEndFrame(impl->session, &frameEndInfo));
			}

			CAGE_FORCE_INLINE void check(XrResult result)
			{
				return impl->check(result);
			}
		};
	}

	bool VirtualRealityController::tracking() const
	{
		const VirtualRealityControllerImpl *impl = (const VirtualRealityControllerImpl *)this;
		return impl->tracking;
	}

	Transform VirtualRealityController::aimPose() const
	{
		const VirtualRealityControllerImpl *impl = (const VirtualRealityControllerImpl *)this;
		return impl->aim.transform;
	}

	Transform VirtualRealityController::gripPose() const
	{
		const VirtualRealityControllerImpl *impl = (const VirtualRealityControllerImpl *)this;
		return impl->grip.transform;
	}

	PointerRange<const Real> VirtualRealityController::axes() const
	{
		const VirtualRealityControllerImpl *impl = (const VirtualRealityControllerImpl *)this;
		return impl->axes;
	}

	PointerRange<const bool> VirtualRealityController::buttons() const
	{
		const VirtualRealityControllerImpl *impl = (const VirtualRealityControllerImpl *)this;
		return impl->buts;
	}

	void VirtualRealityGraphicsFrame::updateProjections()
	{
		VirtualRealityGraphicsFrameImpl *impl = (VirtualRealityGraphicsFrameImpl *)this;
		impl->updateProjections();
	}

	Transform VirtualRealityGraphicsFrame::pose() const
	{
		const VirtualRealityGraphicsFrameImpl *impl = (const VirtualRealityGraphicsFrameImpl *)this;
		return impl->headTransform;
	}

	uint64 VirtualRealityGraphicsFrame::displayTime() const
	{
		const VirtualRealityGraphicsFrameImpl *impl = (const VirtualRealityGraphicsFrameImpl *)this;
		// openxr uses nanoseconds, engine uses microseconds
		return (impl->frameState.predictedDisplayTime - impl->impl->firstFrameTime) / 1000;
	}

	void VirtualRealityGraphicsFrame::renderBegin()
	{
		ProfilingScope profiling("VR render begin");
		VirtualRealityGraphicsFrameImpl *impl = (VirtualRealityGraphicsFrameImpl *)this;
		impl->begin();
	}

	void VirtualRealityGraphicsFrame::acquireTextures()
	{
		ProfilingScope profiling("VR acquire textures");
		VirtualRealityGraphicsFrameImpl *impl = (VirtualRealityGraphicsFrameImpl *)this;
		impl->acquire();
	}

	void VirtualRealityGraphicsFrame::renderCommit()
	{
		ProfilingScope profiling("VR render commit");
		VirtualRealityGraphicsFrameImpl *impl = (VirtualRealityGraphicsFrameImpl *)this;
		impl->commit();
	}

	void VirtualReality::processEvents()
	{
		ProfilingScope profiling("VR process events");
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		impl->pollEvents();
		impl->updateInputs();
	}

	bool VirtualReality::tracking() const
	{
		const VirtualRealityImpl *impl = (const VirtualRealityImpl *)this;
		return impl->tracking;
	}

	Transform VirtualReality::pose() const
	{
		const VirtualRealityImpl *impl = (const VirtualRealityImpl *)this;
		return impl->headTransform;
	}

	VirtualRealityController &VirtualReality::leftController()
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		return impl->controllers[0];
	}

	VirtualRealityController &VirtualReality::rightController()
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		return impl->controllers[1];
	}

	Holder<VirtualRealityGraphicsFrame> VirtualReality::graphicsFrame()
	{
		ProfilingScope profiling("VR graphics frame");
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		return systemMemory().createImpl<VirtualRealityGraphicsFrame, VirtualRealityGraphicsFrameImpl>(impl);
	}

	uint64 VirtualReality::targetFrameTiming() const
	{
		const VirtualRealityImpl *impl = (const VirtualRealityImpl *)this;
		if (impl->haveFbDisplayRefreshRate)
		{
			float fps = 0;
			impl->check(impl->xrGetDisplayRefreshRateFB(impl->session, &fps));
			if (fps > 1)
				return 1000000 / (double)fps;
		}
		return 1000000 / 90;
	}

	Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config)
	{
		return systemMemory().createImpl<VirtualReality, VirtualRealityImpl>(config);
	}
}
