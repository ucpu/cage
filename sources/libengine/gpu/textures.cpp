#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		SamplerImpl::SamplerImpl(const DeviceImpl &device, const SamplerDescriptor &desc)
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
			sampler = device.device.createSamplerUnique(info);
		}

		SamplerImpl::~SamplerImpl() {}

		TextureImpl::TextureImpl(const DeviceImpl &device_, const TextureDescriptor &desc) : device(&device_), resolution(desc.size), mipLevels(desc.mipLevelCount), dimension(desc.dimension), format(desc.format)
		{
			bool isArray = false;
			switch (dimension)
			{
				case TextureDimensionEnum::e2DArray:
				case TextureDimensionEnum::CubeArray:
					isArray = true;
			}
			bool isCube = false;
			switch (dimension)
			{
				case TextureDimensionEnum::Cube:
				case TextureDimensionEnum::CubeArray:
					isCube = true;
			}
			bool is3d = dimension == TextureDimensionEnum::e3D;

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
			imageInfo.extent.width = desc.size[0];
			imageInfo.extent.height = desc.size[1];
			imageInfo.extent.depth = is3d ? desc.size[2] : 1;
			imageInfo.mipLevels = mipLevels;
			imageInfo.arrayLayers = (isArray ? desc.size[2] : 1) * (isCube ? 6 : 1);
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.tiling = vk::ImageTiling::eOptimal;
			imageInfo.usage = convertTextureUsage(desc.usage);
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
			imageInfo.initialLayout = vk::ImageLayout::eUndefined;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VkImage img;
			check("vmaCreateImage", vmaCreateImage(device->allocator, (VkImageCreateInfo *)&imageInfo, &allocInfo, &img, &allocation, &allocatedInfo));
			image = (vk::Image)img;
		}

		TextureImpl::~TextureImpl()
		{
			vmaDestroyImage(device->allocator, (VkImage)image, allocation);
		}

		TextureViewImpl::TextureViewImpl(const Texture &texture, const TextureViewDescriptor &desc) : texture(texture)
		{
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
			viewInfo.subresourceRange.levelCount = desc.mipLevelCount;
			viewInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
			viewInfo.subresourceRange.layerCount = desc.arrayLayerCount;
			view = texture->device->device.createImageViewUnique(viewInfo);
		}

		TextureViewImpl::~TextureViewImpl() {}
	}
}
