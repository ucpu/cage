#include <memory>
#include <vector>

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuInterface.h>

namespace cage
{
	namespace privat
	{
		struct GpuSurface;
	}

	namespace gpuImpl
	{
		struct GpuSurfaceData;

		class BindGroup : private Immovable
		{
		public:
		};

		class BindGroupLayout : private Immovable
		{
		public:
		};

		class Buffer : private Immovable
		{
		public:
			vk::UniqueBuffer buffer;
		};

		class CommandBuffer : private Immovable
		{
		public:
			vk::UniqueCommandBuffer buffer;
		};

		class CommandEncoder : private Immovable
		{
		public:
		};

		class Device : private Immovable
		{
			struct Bootstrap
			{
				vkb::Instance inst;
				vkb::PhysicalDevice phys;
				vkb::Device dev;
				VkQueue qt = nullptr;
				VkQueue qg = nullptr;
				VkQueue qp = nullptr;

				~Bootstrap();
			};
			Bootstrap bootstrap;

			std::vector<std::shared_ptr<GpuSurfaceData>> surfacesCollection;

		public:
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queueTransfer;
			vk::Queue queueGraphics;
			vk::Queue queuePresent;

			Device(Window *window);
			~Device();

			void bootstrapInit(Window *window);
			Holder<privat::GpuSurface> getWindowGpuSurface(Window *window);
		};

		class RenderPassEncoder : private Immovable
		{
		public:
		};

		class RenderPipeline : private Immovable
		{
		public:
		};

		class Sampler : private Immovable
		{
		public:
			vk::UniqueSampler sampler;
		};

		class ShaderModule : private Immovable
		{
		public:
			vk::UniqueShaderModule shader;
		};

		class PipelineLayout : private Immovable
		{
		public:
		};

		class Texture : private Immovable
		{
		public:
		};

		class TextureView : private Immovable
		{
		public:
		};
	}
}
