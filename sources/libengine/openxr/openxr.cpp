#include <cage-core/files.h> // pathExtractFilename, executableFullPathNoExe
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/texture.h>
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
		Holder<Texture> createTextureForOpenXr(uint32 id, uint32 internalFormat, Vec2i resolution);
	}

	namespace
	{
		const ConfigBool confPrintApiLayers("cage/virtualReality/openxrLogApiLayers", false);
		const ConfigBool confPrintExtensions("cage/virtualReality/openxrLogExtensions", false);
		const ConfigBool confEnableValidation("cage/virtualReality/openxrValidationLayer",
#ifdef CAGE_DEBUG
			true
#else
			false
#endif
		);

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

			void check(XrResult result)
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
			}

			void initHandles()
			{
				{
					std::vector<const char *> exts, layers;
					exts.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
					if (confEnableValidation)
					{
						CAGE_LOG(SeverityEnum::Info, "virtualReality", "enabling api validation layer");
						layers.push_back("XR_APILAYER_LUNARG_core_validation");
					}
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
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "orientation tracking: " + (bool)props.trackingProperties.orientationTracking);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "position tracking: " + (bool)props.trackingProperties.positionTracking);
				}

				check(privat::plaformInitSession(instance, systemId, session));

				{
					static constexpr const XrPosef identityPose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0}, .position = {.x = 0, .y = 0, .z = 0} };
					XrReferenceSpaceCreateInfo info;
					init(info, XR_TYPE_REFERENCE_SPACE_CREATE_INFO);
					info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
					info.poseInReferenceSpace = identityPose;
					check(xrCreateReferenceSpace(session, &info, &playSpace));
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
					info.arraySize = 2; // left and right
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
					if (count != 2)
						CAGE_THROW_ERROR(Exception, "virtual reality requires two eyes");
					for (auto &it : viewConfigs)
						init(it, XR_TYPE_VIEW_CONFIGURATION_VIEW);
					check(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &count, viewConfigs.data()));
				}
				const auto &v = viewConfigs[0];

				{
					// assume that properties are the same for both eyes
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view resolution: " + v.recommendedImageRectWidth + "x" + v.recommendedImageRectHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "max view resolution: " + v.maxImageRectWidth + "x" + v.maxImageRectHeight);
					CAGE_LOG(SeverityEnum::Info, "virtualReality", Stringizer() + "recommended view samples: " + v.recommendedSwapchainSampleCount + ", max: " + v.maxSwapchainSampleCount);
				}

				{
					uint32 count = 0;
					check(xrEnumerateSwapchainFormats(session, 0, &count, nullptr));
					std::vector<int64_t> formats;
					formats.resize(count);
					check(xrEnumerateSwapchainFormats(session, count, &count, formats.data()));

					// todo pick format from the enumeration

					createSwapchain(GL_SRGB8_ALPHA8, XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT, colorSwapchain, colorImages, colorTextures);
					createSwapchain(GL_DEPTH_COMPONENT32, XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthSwapchain, depthImages, depthTextures);
				}

				for (uint32 i = 0; i < 2; i++)
				{
					auto &cv = colorViews[i];
					init(cv, XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW);
					cv.subImage.swapchain = colorSwapchain;
					cv.subImage.imageRect.extent.width = v.recommendedImageRectWidth;
					cv.subImage.imageRect.extent.height = v.recommendedImageRectHeight;
					cv.subImage.imageArrayIndex = i;

					auto &dv = depthViews[i];
					init(dv, XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR);
					dv.minDepth = 0;
					dv.maxDepth = 1;
					dv.subImage.swapchain = depthSwapchain;
					dv.subImage.imageRect.extent.width = v.recommendedImageRectWidth;
					dv.subImage.imageRect.extent.height = v.recommendedImageRectHeight;
					dv.subImage.imageArrayIndex = i;

					cv.next = &dv; // chain the depth to the color views

					init(views[i], XR_TYPE_VIEW);
				}
			}

			void initInputs()
			{
				// todo
			}

			void handleEvent(const XrEventDataBuffer &event)
			{
				switch (event.type)
				{
				case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
					instanceLost = true;
					break;
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
				{
					const XrEventDataSessionStateChanged &ev = (const XrEventDataSessionStateChanged &)event;
					sessionState = ev.state;
					switch (sessionState)
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
							check(xrEndSession(session));
							sessionRunning = false;
						}
					} break;
					case XR_SESSION_STATE_LOSS_PENDING:
					case XR_SESSION_STATE_EXITING:
						sessionLost = true;
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
				ScopeLock lock(mutex);
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

			void nextFrame(VirtualRealityGraphicsFrame &frame)
			{
				ScopeLock lock(mutex);
				if (instanceLost || sessionLost || !sessionRunning)
				{
					frame = VirtualRealityGraphicsFrame();
					return;
				}

				// submit rendered images to the headset
				if (rendering)
				{
					for (uint32 i = 0; i < 2; i++)
					{
						colorViews[i].pose = views[i].pose;
						colorViews[i].fov = views[i].fov;
						depthViews[i].nearZ = frame.nearPlane.value;
						depthViews[i].farZ = frame.farPlane.value;
					}

					XrCompositionLayerProjection projectionLayer;
					init(projectionLayer, XR_TYPE_COMPOSITION_LAYER_PROJECTION);
					projectionLayer.space = playSpace;
					projectionLayer.viewCount = 2;
					projectionLayer.views = colorViews.data();
					const XrCompositionLayerBaseHeader *submittedLayer = (XrCompositionLayerBaseHeader *)&projectionLayer;

					XrFrameEndInfo frameEndInfo;
					init(frameEndInfo, XR_TYPE_FRAME_END_INFO);

					if (frameState.shouldRender)
					{
						check(xrReleaseSwapchainImage(colorSwapchain, nullptr));
						check(xrReleaseSwapchainImage(depthSwapchain, nullptr));

						frameEndInfo.layerCount = 1;
						frameEndInfo.layers = &submittedLayer;
					}

					frameEndInfo.displayTime = frameState.predictedDisplayTime;
					frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
					check(xrEndFrame(session, &frameEndInfo));
					rendering = false;
				}

				// reset to empty
				frame = VirtualRealityGraphicsFrame();

				// prepare data for rendering next frame
				{
					init(frameState, XR_TYPE_FRAME_STATE);
					check(xrWaitFrame(session, nullptr, &frameState)); // blocking call

					if (frameState.shouldRender)
					{
						frame.colorTexture = acquireTexture(colorSwapchain, colorTextures);
						frame.depthTexture = acquireTexture(depthSwapchain, depthTextures);
						frame.resolution = Vec2i(viewConfigs[0].recommendedImageRectWidth, viewConfigs[0].recommendedImageRectHeight);
					}

					XrViewLocateInfo viewLocateInfo;
					init(viewLocateInfo, XR_TYPE_VIEW_LOCATE_INFO);
					viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					viewLocateInfo.displayTime = frameState.predictedDisplayPeriod;
					viewLocateInfo.space = playSpace;
					XrViewState viewState;
					init(viewState, XR_TYPE_VIEW_STATE);
					uint32 count = 0;
					check(xrLocateViews(session, &viewLocateInfo, &viewState, 2, &count, views.data()));

					check(xrBeginFrame(session, nullptr));
					rendering = true;
				}
			}

			Holder<Mutex> mutex = newMutex();

			XrInstance instance = XR_NULL_HANDLE;
			XrSystemId systemId = XR_NULL_SYSTEM_ID;
			XrSession session = XR_NULL_HANDLE;
			XrSpace playSpace = XR_NULL_HANDLE;

			std::array<XrViewConfigurationView, 2> viewConfigs = {};
			XrSwapchain colorSwapchain, depthSwapchain;
			std::vector<XrSwapchainImageOpenGLKHR> colorImages, depthImages;
			std::vector<Holder<Texture>> colorTextures, depthTextures;
			std::array<XrCompositionLayerProjectionView, 2> colorViews;
			std::array<XrCompositionLayerDepthInfoKHR, 2> depthViews;
			std::array<XrView, 2> views;

			XrFrameState frameState = {};
			XrSessionState sessionState = XrSessionState::XR_SESSION_STATE_UNKNOWN;
			bool sessionRunning = false;
			bool sessionLost = false;
			bool instanceLost = false;
			bool rendering = false;
		};
	}

	void VirtualReality::processEvents()
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		impl->pollEvents();
	}

	void VirtualReality::graphicsFrame(VirtualRealityGraphicsFrame &frame)
	{
		VirtualRealityImpl *impl = (VirtualRealityImpl *)this;
		impl->nextFrame(frame);
	}

	Holder<VirtualReality> newVirtualReality(const VirtualRealityCreateConfig &config)
	{
		return systemMemory().createImpl<VirtualReality, VirtualRealityImpl>(config);
	}
}
