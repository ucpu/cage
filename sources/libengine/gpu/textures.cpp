#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		template<>
		void ResourceInternal<vk::Sampler, Nothing>::destroy()
		{
			device->device.destroySampler(value);
		}

		template<>
		void ResourceInternal<vk::Image, VmaAllocation>::destroy()
		{
			if (extra)
				vmaDestroyImage(device->allocator, (VkImage)value, extra);
		}

		template<>
		void ResourceInternal<vk::ImageView, Nothing>::destroy()
		{
			device->device.destroyImageView(value);
		}

		SamplerImpl::SamplerImpl(DeviceImpl &device, const SamplerDescriptor &desc) : sampler(device)
		{
			vk::SamplerCreateInfo info;
			info.magFilter = convertFilter(desc.magFilter);
			info.minFilter = convertFilter(desc.minFilter);
			info.mipmapMode = convertMipmapFilter(desc.mipmapFilter);
			info.addressModeU = convertAddressMode(desc.addressModeU);
			info.addressModeV = convertAddressMode(desc.addressModeV);
			info.addressModeW = convertAddressMode(desc.addressModeW);
			info.anisotropyEnable = desc.maxAnisotropy > 1;
			info.maxAnisotropy = desc.maxAnisotropy;
			sampler = device.device.createSampler(info);
			sampler.setLabel(desc.label);
		}

		SamplerImpl::~SamplerImpl() {}

		TextureImpl::TextureImpl(DeviceImpl &device, vk::Image image_) : image(device)
		{
			image = std::move(image_);
		}

		TextureImpl::TextureImpl(DeviceImpl &device, const TextureDescriptor &desc) : image(device), resolution(desc.resolution), arrayLayers(desc.arrayLayers), mipLevels(desc.mipLevels), dimension(desc.dimension), format(desc.format), usage(desc.usage)
		{
			CAGE_ASSERT(desc.dimension != TextureDimensionEnum::Undefined);

			bool isCube = false;
			switch (dimension)
			{
				case TextureDimensionEnum::Cube:
				case TextureDimensionEnum::CubeArray:
					isCube = true;
			}

			vk::ImageCreateInfo imageInfo;
			switch (dimension)
			{
				case TextureDimensionEnum::e1D:
					imageInfo.imageType = vk::ImageType::e1D;
					break;
				case TextureDimensionEnum::e2D:
				case TextureDimensionEnum::e2DArray:
				case TextureDimensionEnum::Cube:
				case TextureDimensionEnum::CubeArray:
					imageInfo.imageType = vk::ImageType::e2D;
					break;
				case TextureDimensionEnum::e3D:
					imageInfo.imageType = vk::ImageType::e3D;
					break;
			}

			if (isCube)
				imageInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
			imageInfo.format = convertTextureFormat(format);
			imageInfo.extent.width = resolution[0];
			imageInfo.extent.height = resolution[1];
			imageInfo.extent.depth = resolution[2];
			imageInfo.arrayLayers = arrayLayers;
			imageInfo.mipLevels = mipLevels;
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.tiling = vk::ImageTiling::eOptimal;
			imageInfo.usage = convertTextureUsage(usage, format);
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
			imageInfo.initialLayout = vk::ImageLayout::eUndefined;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VkImage img;
			check("vmaCreateImage", vmaCreateImage(device.allocator, (VkImageCreateInfo *)&imageInfo, &allocInfo, &img, &image.holder->extra, &allocatedInfo));
			image = (vk::Image)img;
			image.setLabel(desc.label);
		}

		TextureImpl::~TextureImpl() {}

		TextureViewImpl::TextureViewImpl(const Texture &texture, const TextureViewDescriptor &desc) : view(*texture->image.device()), texture(texture)
		{
			CAGE_ASSERT(desc.dimension != TextureDimensionEnum::Undefined);

			vk::ImageViewCreateInfo viewInfo;
			switch (desc.dimension)
			{
				case TextureDimensionEnum::e2D:
					viewInfo.viewType = vk::ImageViewType::e2D;
					break;
				case TextureDimensionEnum::e2DArray:
					viewInfo.viewType = vk::ImageViewType::e2DArray;
					break;
				case TextureDimensionEnum::Cube:
					viewInfo.viewType = vk::ImageViewType::eCube;
					break;
				case TextureDimensionEnum::CubeArray:
					viewInfo.viewType = vk::ImageViewType::eCubeArray;
					break;
				case TextureDimensionEnum::e3D:
					viewInfo.viewType = vk::ImageViewType::e3D;
					break;
			}

			viewInfo.image = texture->image;
			viewInfo.format = convertTextureFormat(texture->format);
			viewInfo.subresourceRange.aspectMask = convertAspectMask(texture->format);
			viewInfo.subresourceRange.baseMipLevel = desc.baseMipLevel;
			viewInfo.subresourceRange.levelCount = desc.mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
			viewInfo.subresourceRange.layerCount = desc.arrayLayers;
			view = texture->image.device()->device.createImageView(viewInfo);
			view.setLabel(desc.label);
		}

		TextureViewImpl::~TextureViewImpl() {}
	}
}
