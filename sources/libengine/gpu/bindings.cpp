#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		BindGroupImpl::BindGroupImpl(const DeviceImpl &device, const BindGroupDescriptor &desc)
		{
			vk::DescriptorSetAllocateInfo allocInfo;
			allocInfo.descriptorPool = *device.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &*desc.layout->layout;
			auto sets = device.device.allocateDescriptorSets(allocInfo);
			set = makeUnique<vk::DescriptorSet>(sets[0]);

			ankerl::svector<vk::WriteDescriptorSet, 10> writes;
			ankerl::svector<vk::DescriptorBufferInfo, 10> infosBuffers;
			ankerl::svector<vk::DescriptorImageInfo, 10> infosImages;
			writes.reserve(desc.entries.size());
			infosBuffers.reserve(desc.entries.size());
			infosImages.reserve(desc.entries.size());
			for (const auto &entry : desc.entries)
			{
				vk::WriteDescriptorSet write;
				write.dstSet = *set;
				write.dstBinding = entry.binding;
				write.dstArrayElement = 0;
				write.descriptorCount = 1;
				std::visit(
					[&](auto &e)
					{
						using T = std::decay_t<decltype(e)>;
						if constexpr (std::is_same_v<T, BindGroupDescriptor::BufferEntry>)
						{
							vk::DescriptorBufferInfo &bufferInfo = infosBuffers.emplace_back(); // make sure the struct outlives its use
							bufferInfo.buffer = *e.buffer->buffer;
							bufferInfo.offset = e.offset;
							bufferInfo.range = e.size;
							// write.descriptorType = vk::DescriptorType::eUniformBuffer; // todo
							write.pBufferInfo = &bufferInfo;
						}
						else if constexpr (std::is_same_v<T, BindGroupDescriptor::SamplerEntry>)
						{
							vk::DescriptorImageInfo &imageInfo = infosImages.emplace_back(); // make sure the struct outlives its use
							imageInfo.sampler = *e.sampler->sampler;
							write.descriptorType = vk::DescriptorType::eSampler;
							write.pImageInfo = &imageInfo;
						}
						else if constexpr (std::is_same_v<T, BindGroupDescriptor::TextureEntry>)
						{
							vk::DescriptorImageInfo &imageInfo = infosImages.emplace_back(); // make sure the struct outlives its use
							imageInfo.imageView = *e.textureView->view;
							write.descriptorType = vk::DescriptorType::eSampledImage;
							write.pImageInfo = &imageInfo;
						}
					},
					entry.data);
				writes.push_back(write);
			}

			device.device.updateDescriptorSets(writes, {});
		}

		BindGroupImpl::~BindGroupImpl() {}

		BindGroupLayoutImpl::BindGroupLayoutImpl(const DeviceImpl &device, const BindGroupLayoutDescriptor &desc)
		{
			ankerl::svector<vk::DescriptorSetLayoutBinding, 10> bindings;
			bindings.reserve(desc.entries.size());
			for (const auto &entry : desc.entries)
			{
				vk::DescriptorSetLayoutBinding b;
				b.binding = entry.binding;
				b.descriptorCount = 1;
				b.stageFlags = convertShaderStages(entry.visibility);
				std::visit(
					[&](auto &e)
					{
						using T = std::decay_t<decltype(e)>;
						if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::BufferEntry>)
						{
							switch (e.type)
							{
								case BufferBindingTypeEnum::Uniform:
									b.descriptorType = e.hasDynamicOffset ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
									break;
								case BufferBindingTypeEnum::Storage:
								case BufferBindingTypeEnum::ReadOnlyStorage:
									b.descriptorType = e.hasDynamicOffset ? vk::DescriptorType::eStorageBufferDynamic : vk::DescriptorType::eStorageBuffer;
									break;
							}
						}
						else if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::SamplerEntry>)
						{
							b.descriptorType = vk::DescriptorType::eSampler;
						}
						else if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::TextureEntry>)
						{
							b.descriptorType = vk::DescriptorType::eSampledImage;
						}
					},
					entry.data);
				bindings.push_back(std::move(b));
			}

			vk::DescriptorSetLayoutCreateInfo ci;
			ci.bindingCount = static_cast<uint32_t>(bindings.size());
			ci.pBindings = bindings.data();
			layout = device.device.createDescriptorSetLayoutUnique(ci);
		}

		BindGroupLayoutImpl::~BindGroupLayoutImpl() {}
	}
}
