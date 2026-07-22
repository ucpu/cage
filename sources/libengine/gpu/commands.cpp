#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		template<>
		void ResourceInternal<std::vector<Holder<void>>, Nothing>::destroy()
		{
			value.clear();
		}

		template<>
		void ResourceInternal<vk::CommandPool, Nothing>::destroy()
		{
			device->device.destroyCommandPool(value);
		}

		template<>
		void ResourceInternal<vk::CommandBuffer, vk::CommandPool>::destroy()
		{
			device->device.freeCommandBuffers(extra, value);
		}

		CommandBufferImpl::CommandBufferImpl(DeviceImpl &device) : pool(device), buffer(device), rtka(device)
		{
			vk::CommandPoolCreateInfo info1;
			pool = device.device.createCommandPool(info1);

			vk::CommandBufferAllocateInfo info2;
			info2.commandBufferCount = 1;
			info2.commandPool = pool;
			buffer = std::move(device.device.allocateCommandBuffers(info2)[0]);
			buffer = (vk::CommandPool)pool;
		}

		CommandBufferImpl::~CommandBufferImpl() {}

		CommandEncoderImpl::CommandEncoderImpl(DeviceImpl &device, const CommandEncoderDescriptor &desc) : buffer(CommandBuffer(systemMemory().createHolder<CommandBufferImpl>(device))), cmd(buffer->buffer)
		{
			buffer->buffer.setLabel(desc.label);

			vk::CommandBufferBeginInfo info;
			info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			cmd.begin(info);
		}

		CommandEncoderImpl::~CommandEncoderImpl() {}

		void CommandEncoderImpl::imageTransitionPermanent(const Texture &texture, ImageStateEnum sourceState, ImageStateEnum targetState)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			CAGE_ASSERT(targetState != ImageStateEnum::Undefined);

			vk::ImageMemoryBarrier2 barrier;
			barrier.image = texture->image;
			barrier.subresourceRange.aspectMask = convertAspectMask(texture->format);
			barrier.subresourceRange.layerCount = texture->arrayLayersCount;
			barrier.subresourceRange.levelCount = texture->mipLevelsCount;
			assignImageStateBarrierFlags(sourceState, barrier.srcStageMask, barrier.srcAccessMask, barrier.oldLayout);
			assignImageStateBarrierFlags(targetState, barrier.dstStageMask, barrier.dstAccessMask, barrier.newLayout);
			vk::DependencyInfo dependency;
			dependency.imageMemoryBarrierCount = 1;
			dependency.pImageMemoryBarriers = &barrier;
			cmd.pipelineBarrier2(&dependency);

			texture->defaultState = targetState;
		}

		void CommandEncoderImpl::imageTransitionSubresource(const ImageTransitionSubresource &subres, ImageStateEnum targetState)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			CAGE_ASSERT(targetState != ImageStateEnum::Undefined);

			ImageStateEnum sourceState = ImageStateEnum::Undefined;
			{
				auto it = synchronizedImages.find(subres);
				if (it == synchronizedImages.end())
					sourceState = subres.texture->defaultState;
				else
					sourceState = it->second;
				if (sourceState == targetState)
					return; // already in the correct state
			}

			{
				vk::ImageMemoryBarrier2 barrier;
				barrier.image = subres.texture->image;
				barrier.subresourceRange.aspectMask = convertAspectMask(subres.texture->format);
				barrier.subresourceRange.baseArrayLayer = subres.layer;
				barrier.subresourceRange.layerCount = 1;
				barrier.subresourceRange.baseMipLevel = subres.mip;
				barrier.subresourceRange.levelCount = 1;
				assignImageStateBarrierFlags(sourceState, barrier.srcStageMask, barrier.srcAccessMask, barrier.oldLayout);
				assignImageStateBarrierFlags(targetState, barrier.dstStageMask, barrier.dstAccessMask, barrier.newLayout);
				vk::DependencyInfo dependency;
				dependency.imageMemoryBarrierCount = 1;
				dependency.pImageMemoryBarriers = &barrier;
				cmd.pipelineBarrier2(&dependency);
			}

			if (subres.texture->defaultState == targetState)
				synchronizedImages.erase(subres);
			else
				synchronizedImages[subres] = targetState;
		}

		void CommandEncoderImpl::imageTransitionSubresource(const TextureView &view, ImageStateEnum targetState)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			CAGE_ASSERT(targetState != ImageStateEnum::Undefined);
			CAGE_ASSERT(view->arrayLayersCount == 1 && view->mipLevelsCount == 1);
			ImageTransitionSubresource s;
			s.texture = view->texture;
			s.mip = view->mipLevelsOffset;
			s.layer = view->arrayLayersOffset;
			imageTransitionSubresource(std::move(s), targetState);
		}

		void CommandEncoderImpl::bufferSynchronizationPermanent(const Buffer &buffer, BufferStateEnum sourceState, BufferStateEnum targetState)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			CAGE_ASSERT(targetState != BufferStateEnum::Undefined);

			vk::BufferMemoryBarrier2 barrier;
			barrier.buffer = buffer->buffer;
			barrier.offset = 0;
			barrier.size = VK_WHOLE_SIZE;
			assignBufferStateBarrierFlags(sourceState, barrier.srcStageMask, barrier.srcAccessMask);
			assignBufferStateBarrierFlags(targetState, barrier.dstStageMask, barrier.dstAccessMask);
			vk::DependencyInfo dependency;
			dependency.bufferMemoryBarrierCount = 1;
			dependency.pBufferMemoryBarriers = &barrier;
			cmd.pipelineBarrier2(dependency);

			buffer->defaultState = targetState;
		}

		void CommandEncoderImpl::bufferSynchronization(const Buffer &buffer, BufferStateEnum targetState)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			CAGE_ASSERT(targetState != BufferStateEnum::Undefined);

			BufferStateEnum sourceState = BufferStateEnum::Undefined;
			{
				auto it = synchronizedBuffers.find(buffer);
				if (it == synchronizedBuffers.end())
					sourceState = buffer->defaultState;
				else
					sourceState = it->second;
				if (sourceState == targetState)
					return; // already in the correct state
			}

			{
				vk::BufferMemoryBarrier2 barrier;
				barrier.buffer = buffer->buffer;
				barrier.offset = 0;
				barrier.size = VK_WHOLE_SIZE;
				assignBufferStateBarrierFlags(sourceState, barrier.srcStageMask, barrier.srcAccessMask);
				assignBufferStateBarrierFlags(targetState, barrier.dstStageMask, barrier.dstAccessMask);
				vk::DependencyInfo dependency;
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &barrier;
				cmd.pipelineBarrier2(dependency);
			}

			if (buffer->defaultState == targetState)
				synchronizedBuffers.erase(buffer);
			else
				synchronizedBuffers[buffer] = targetState;
		}

		void CommandEncoderImpl::resetSynchronization()
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);

			while (!synchronizedImages.empty())
			{
				const ImageTransitionSubresource t = synchronizedImages.begin()->first; // make a copy, it will be erased from the array
				imageTransitionSubresource(t, t.texture->defaultState);
			}

			while (!synchronizedBuffers.empty())
			{
				Buffer b = synchronizedBuffers.begin()->first; // make a copy, it will be erased from the array
				bufferSynchronization(b, b->defaultState);
			}
		}

		void CommandEncoderImpl::pushDebugGroup(StringView label)
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			vk::DebugUtilsLabelEXT info;
			info.pLabelName = label.str.data();
			cmd.beginDebugUtilsLabelEXT(info);
		}

		void CommandEncoderImpl::popDebugGroup()
		{
			CAGE_ASSERT(currentMode != EncoderModeEnum::Undefined);
			cmd.endDebugUtilsLabelEXT();
		}

		CommandBuffer CommandEncoderImpl::finishEncoding()
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);
			resetSynchronization();
			currentMode = EncoderModeEnum::Undefined;
			cmd.end();
			return std::move(buffer);
		}

		void CommandEncoderImpl::copyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);

			bufferSynchronization(source, BufferStateEnum::Read);
			bufferSynchronization(destination, BufferStateEnum::Write);

			vk::BufferCopy2 region;
			region.srcOffset = sourceOffset;
			region.dstOffset = destinationOffset;
			region.size = size;
			vk::CopyBufferInfo2 info;
			info.srcBuffer = source->buffer;
			info.dstBuffer = destination->buffer;
			info.regionCount = 1;
			info.pRegions = &region;
			cmd.copyBuffer2(info);
		}

		void CommandEncoderImpl::copyBufferToTexture(const TexelCopyBufferInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);

			bufferSynchronization(source.buffer, BufferStateEnum::Read);
			for (uint32 i = 0; i < destination.arrayLayersCount; i++)
			{
				ImageTransitionSubresource subres;
				subres.texture = destination.texture;
				subres.mip = destination.mipLevel;
				subres.layer = destination.arrayLayersOffset + i;
				imageTransitionSubresource(subres, ImageStateEnum::TransferDst);
			}

			vk::BufferImageCopy2 region;
			region.bufferOffset = source.layout.offset;
			region.bufferRowLength = copySize[0];
			region.bufferImageHeight = copySize[1];
			region.imageOffset.x = destination.origin[0];
			region.imageOffset.y = destination.origin[1];
			region.imageOffset.z = destination.origin[2];
			region.imageExtent.width = copySize[0];
			region.imageExtent.height = copySize[1];
			region.imageExtent.depth = copySize[2];
			region.imageSubresource.aspectMask = convertAspectMask(destination.texture.getFormat());
			region.imageSubresource.baseArrayLayer = destination.arrayLayersOffset;
			region.imageSubresource.layerCount = destination.arrayLayersCount;
			region.imageSubresource.mipLevel = destination.mipLevel;
			vk::CopyBufferToImageInfo2 info;
			info.srcBuffer = source.buffer->buffer;
			info.dstImage = destination.texture->image;
			info.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
			info.regionCount = 1;
			info.pRegions = &region;
			cmd.copyBufferToImage2(info);
		}

		void CommandEncoderImpl::copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);

			// todo
		}

		void CommandEncoderImpl::beginRenderPass(const RenderPassDescriptor &desc)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);
			currentMode = EncoderModeEnum::Rendering;

			resetSynchronization();

			vk::Rect2D rect;
			ankerl::svector<vk::RenderingAttachmentInfo, 4> attachments;
			for (const auto &it : desc.colorAttachments)
			{
				vk::RenderingAttachmentInfo info;
				// todo info.clearValue;
				info.imageView = it.view->view;
				info.loadOp = convertLoadOperation(it.loadOp);
				info.storeOp = convertStoreOperation(it.storeOp);
				info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
				attachments.push_back(std::move(info));
				const auto res = it.view->texture.getResolution();
				rect.extent.width = res[0];
				rect.extent.height = res[1];
				keepAlive(it.view);
				imageTransitionSubresource(it.view, ImageStateEnum::ColorAttachment);
			}

			vk::RenderingAttachmentInfo depth;
			vk::RenderingAttachmentInfo stencil;
			if (desc.depthStencilAttachment)
			{
				// todo depth.clearValue;
				depth.imageView = desc.depthStencilAttachment->view->view;
				depth.loadOp = convertLoadOperation(desc.depthStencilAttachment->depthLoadOp);
				depth.storeOp = convertStoreOperation(desc.depthStencilAttachment->depthStoreOp);
				depth.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
				const auto res = desc.depthStencilAttachment->view->texture.getResolution();
				rect.extent.width = res[0];
				rect.extent.height = res[1];
				keepAlive(desc.depthStencilAttachment->view);
				imageTransitionSubresource(desc.depthStencilAttachment->view, ImageStateEnum::DepthAttachment);
			}

			vk::RenderingInfo info;
			info.colorAttachmentCount = attachments.size();
			info.pColorAttachments = attachments.data();
			info.pDepthAttachment = &depth;
			info.pStencilAttachment = &stencil;
			info.renderArea = rect;
			info.layerCount = 1;
			cmd.beginRendering(info);
		}

		void CommandEncoderImpl::endRenderPass()
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			currentMode = EncoderModeEnum::Generic;
			cmd.endRendering();
			resetSynchronization();
		}

		void CommandEncoderImpl::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			vk::Rect2D rect = vk::Rect2D({ (sint32)x, (sint32)y }, { w, h });
			cmd.setScissor(0, rect);
		}

		void CommandEncoderImpl::setPipeline(const RenderPipeline &pipeline)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			keepAlive(pipeline);
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);
			currentPipelineLayout = pipeline->layout->layout;
		}

		void CommandEncoderImpl::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			keepAlive(group);
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPipelineLayout, binding, (vk::DescriptorSet)group->set, dynamicOffsets);
		}

		void CommandEncoderImpl::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			keepAlive(buffer);
			cmd.bindVertexBuffers(slot, (vk::Buffer)buffer->buffer, offset);
		}

		void CommandEncoderImpl::setViewport(Real x, Real y, Real width, Real height, Real minDepth, Real maxDepth)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			vk::Viewport vp;
			vp.x = x.value;
			vp.y = y.value;
			vp.width = width.value;
			vp.height = -height.value;
			vp.minDepth = minDepth.value;
			vp.maxDepth = maxDepth.value;
			cmd.setViewport(0, 1, &vp);
		}

		void CommandEncoderImpl::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			keepAlive(buffer);
			cmd.bindIndexBuffer(buffer->buffer, offset, convertIndexFormat(format));
		}

		void CommandEncoderImpl::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd.drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void CommandEncoderImpl::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd.draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}
	}
}
