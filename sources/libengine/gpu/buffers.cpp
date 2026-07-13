#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		BufferImpl::BufferImpl(DeviceImpl &device_, const BufferDescriptor &desc) : device(&device_), size(desc.size), usage(desc.usage)
		{
			vk::BufferCreateInfo bufferInfo;
			bufferInfo.size = desc.size;
			bufferInfo.usage = convertBufferUsage(desc.usage);
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			allocInfo.usage = any(desc.usage & (BufferUsageFlags::MapRead | BufferUsageFlags::MapWrite)) ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VkBuffer buff;
			check("vmaCreateBuffer", vmaCreateBuffer(device->allocator, (VkBufferCreateInfo *)&bufferInfo, &allocInfo, &buff, &allocation, &allocatedInfo));
			buffer = (vk::Buffer)buff;

			if (allocatedInfo.pMappedData)
				mappedRange = PointerRange((char *)allocatedInfo.pMappedData, (char *)allocatedInfo.pMappedData + size);
		}

		BufferImpl::~BufferImpl()
		{
			vmaDestroyBuffer(device->allocator, (VkBuffer)buffer, allocation);
		}

		void BufferImpl::flush()
		{
			check("vmaFlushAllocation", vmaFlushAllocation(device->allocator, allocation, 0, size));
		}

		void BufferImpl::invalidate()
		{
			check("vmaInvalidateAllocation", vmaInvalidateAllocation(device->allocator, allocation, 0, size));
		}
	}
}
