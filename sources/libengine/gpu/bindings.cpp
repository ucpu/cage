#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		template<>
		void ResourceInternal<vk::DescriptorSet, vk::DescriptorPool>::destroy()
		{
			device->device.freeDescriptorSets(extra, value);
		}

		template<>
		void ResourceInternal<vk::DescriptorSetLayout, Nothing>::destroy()
		{
			device->device.destroyDescriptorSetLayout(value);
		}

		namespace
		{
			const BindGroupLayoutDescriptor::Entry &findBinding(const BindGroupLayout &layout, uint32 binding)
			{
				for (const auto &it : layout->entries)
				{
					if (it.binding == binding)
						return it;
				}
				CAGE_THROW_ERROR(Exception, "missing binding layout entry");
			}
		}

		BindGroupImpl::BindGroupImpl(DeviceImpl &device, const BindGroupDescriptor &desc) : set(device), rtka(device)
		{
			vk::DescriptorSetAllocateInfo allocInfo;
			allocInfo.descriptorPool = *device.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &(vk::DescriptorSetLayout &)desc.layout->layout;
			auto sets = device.device.allocateDescriptorSets(allocInfo);
			set = std::move(sets[0]);
			set.setLabel(desc.label);
			set = (vk::DescriptorPool)*device.descriptorPool;

			ankerl::svector<vk::WriteDescriptorSet, 32> writes;
			ankerl::svector<vk::DescriptorBufferInfo, 24> infosBuffers;
			ankerl::svector<vk::DescriptorImageInfo, 24> infosImages;
			writes.reserve(desc.entries.size());
			infosBuffers.reserve(desc.entries.size());
			infosImages.reserve(desc.entries.size());
			for (const auto &entry : desc.entries)
			{
				CAGE_ASSERT(entry.binding != m);
				vk::WriteDescriptorSet write;
				write.dstSet = set;
				write.dstBinding = entry.binding;
				write.dstArrayElement = 0;
				write.descriptorCount = 1;
				std::visit(
					[&](auto &e)
					{
						using T = std::decay_t<decltype(e)>;
						if constexpr (std::is_same_v<T, std::monostate>)
						{
							CAGE_ASSERT(!"empty BindGroupDescriptor entry");
						}
						else if constexpr (std::is_same_v<T, BindGroupDescriptor::BufferEntry>)
						{
							CAGE_ASSERT(e.offset <= e.buffer.getSize());
							CAGE_ASSERT(e.size == m || e.offset + e.size <= e.buffer.getSize());
							vk::DescriptorBufferInfo &bufferInfo = infosBuffers.emplace_back(); // make sure the struct outlives its use
							bufferInfo.buffer = e.buffer->buffer;
							bufferInfo.offset = e.offset;
							bufferInfo.range = e.size == m ? e.buffer.getSize() - e.offset : e.size;
							const auto l = findBinding(desc.layout, entry.binding);
							CAGE_ASSERT(std::holds_alternative<BindGroupLayoutDescriptor::BufferEntry>(l.data));
							const auto &lb = std::get<BindGroupLayoutDescriptor::BufferEntry>(l.data);
							write.descriptorType = convertBindingBufferType(lb.type, lb.hasDynamicOffset);
							write.pBufferInfo = &bufferInfo;
							rtka.keepAlive(e.buffer);
						}
						else if constexpr (std::is_same_v<T, BindGroupDescriptor::SamplerEntry>)
						{
							vk::DescriptorImageInfo &imageInfo = infosImages.emplace_back(); // make sure the struct outlives its use
							imageInfo.sampler = e.sampler->sampler;
							write.descriptorType = vk::DescriptorType::eSampler;
							write.pImageInfo = &imageInfo;
							rtka.keepAlive(e.sampler);
						}
						else if constexpr (std::is_same_v<T, BindGroupDescriptor::TextureEntry>)
						{
							CAGE_ASSERT(e.textureView->texture->defaultState == ImageStateEnum::Sampled);
							vk::DescriptorImageInfo &imageInfo = infosImages.emplace_back(); // make sure the struct outlives its use
							imageInfo.imageView = e.textureView->view;
							imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
							write.descriptorType = vk::DescriptorType::eSampledImage;
							write.pImageInfo = &imageInfo;
							rtka.keepAlive(e.textureView);
						}
						else
						{
							static_assert([] { return false; }(), "unknown BindGroupDescriptor entry type");
						}
					},
					entry.data);
				writes.push_back(write);
			}

			device.device.updateDescriptorSets(writes, {});
		}

		BindGroupImpl::~BindGroupImpl() {}

		BindGroupLayoutImpl::BindGroupLayoutImpl(DeviceImpl &device, const BindGroupLayoutDescriptor &desc) : layout(device)
		{
			ankerl::svector<vk::DescriptorSetLayoutBinding, 32> bindings;
			bindings.reserve(desc.entries.size());
			for (const auto &entry : desc.entries)
			{
				CAGE_ASSERT(entry.binding != m);
				vk::DescriptorSetLayoutBinding b;
				b.binding = entry.binding;
				b.descriptorCount = 1;
				b.stageFlags = convertShaderStages(entry.visibility);
				std::visit(
					[&](auto &e)
					{
						using T = std::decay_t<decltype(e)>;
						if constexpr (std::is_same_v<T, std::monostate>)
						{
							CAGE_ASSERT(!"empty BindGroupLayoutDescriptor entry");
						}
						else if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::BufferEntry>)
						{
							b.descriptorType = convertBindingBufferType(e.type, e.hasDynamicOffset);
						}
						else if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::SamplerEntry>)
						{
							b.descriptorType = vk::DescriptorType::eSampler;
						}
						else if constexpr (std::is_same_v<T, BindGroupLayoutDescriptor::TextureEntry>)
						{
							b.descriptorType = vk::DescriptorType::eSampledImage;
						}
						else
						{
							static_assert([] { return false; }(), "unknown BindGroupLayoutDescriptor entry type");
						}
					},
					entry.data);
				bindings.push_back(std::move(b));
			}

			vk::DescriptorSetLayoutCreateInfo ci;
			ci.bindingCount = bindings.size();
			ci.pBindings = bindings.data();
			layout = device.device.createDescriptorSetLayout(ci);
			layout.setLabel(desc.label);

			entries.reserve(desc.entries.size());
			for (auto &it : desc.entries)
				entries.push_back(it);
		}

		BindGroupLayoutImpl::~BindGroupLayoutImpl() {}
	}
}
