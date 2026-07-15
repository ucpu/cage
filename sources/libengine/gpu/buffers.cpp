#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		template<>
		void deferredDestructor(ResourceHandle<vk::Buffer, VmaAllocation> &handle)
		{
			if (handle.extra)
				vmaDestroyBuffer(handle.device->allocator, (VkBuffer)handle.value, handle.extra);
		}

		BufferImpl::BufferImpl(DeviceImpl &device, const BufferDescriptor &desc) : size(desc.size), usage(desc.usage)
		{
			vk::BufferCreateInfo bufferInfo;
			bufferInfo.size = desc.size;
			bufferInfo.usage = convertBufferUsage(desc.usage);
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			allocInfo.usage = any(desc.usage & (BufferUsageFlags::MapRead | BufferUsageFlags::MapWrite)) ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VkBuffer buff;
			check("vmaCreateBuffer", vmaCreateBuffer(device.allocator, (VkBufferCreateInfo *)&bufferInfo, &allocInfo, &buff, &buffer.extra, &allocatedInfo));
			buffer.device = &device;
			buffer.value = (vk::Buffer)buff;
			buffer.setLabel(desc.label);

			if (allocatedInfo.pMappedData)
				mappedRange = PointerRange((char *)allocatedInfo.pMappedData, (char *)allocatedInfo.pMappedData + size);
		}

		BufferImpl::~BufferImpl() {}

		void BufferImpl::flush()
		{
			check("vmaFlushAllocation", vmaFlushAllocation(buffer.device->allocator, buffer.extra, 0, size));
		}

		void BufferImpl::invalidate()
		{
			check("vmaInvalidateAllocation", vmaInvalidateAllocation(buffer.device->allocator, buffer.extra, 0, size));
		}
	}
}
