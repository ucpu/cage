#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		template<>
		void deferredDestructor(ResourceHandle<vk::CommandBuffer, Nothing> &handle)
		{
			// todo
		}

		CommandBufferImpl::CommandBufferImpl() {}

		CommandBufferImpl::~CommandBufferImpl() {}

		CommandEncoderImpl::CommandEncoderImpl(DeviceImpl &device, const CommandEncoderDescriptor &desc)
		{
			cmd.device = &device;
			cmd.value = device.newCommandBuffer();
			cmd.setLabel(desc.label);

			vk::CommandBufferBeginInfo info;
			info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			cmd->begin(info);
		}

		CommandEncoderImpl::~CommandEncoderImpl() {}

		void CommandEncoderImpl::imageTransition(const Texture &texture, ImageStateEnum targetState, bool permanent)
		{
			ImageStateEnum sourceState = ImageStateEnum::Undefined;
			{
				auto it = transitionedImages.find(texture);
				if (it == transitionedImages.end())
					sourceState = texture->defaultState;
				else
					sourceState = it->second;
				if (sourceState == targetState)
				{ // already in the correct state
					if (permanent)
					{
						texture->defaultState = targetState;
						transitionedImages.erase(texture);
					}
					return;
				}
			}

			{
				vk::ImageMemoryBarrier2 imgBar;
				imgBar.image = texture->image;
				imgBar.subresourceRange.aspectMask = convertAspectMask(texture->format);
				imgBar.subresourceRange.layerCount = texture->arrayLayers;
				imgBar.subresourceRange.levelCount = texture->mipLevels;
				assignImageStateBarrierFlags(sourceState, imgBar.srcStageMask, imgBar.srcAccessMask, imgBar.oldLayout);
				assignImageStateBarrierFlags(targetState, imgBar.dstStageMask, imgBar.dstAccessMask, imgBar.newLayout);

				vk::DependencyInfo barInfo;
				barInfo.imageMemoryBarrierCount = 1;
				barInfo.pImageMemoryBarriers = &imgBar;
				cmd->pipelineBarrier2(&barInfo);
			}

			if (permanent)
				texture->defaultState = targetState;
			if (texture->defaultState == targetState)
				transitionedImages.erase(texture);
			else
				transitionedImages[texture] = targetState;
		}

		void CommandEncoderImpl::pushDebugGroup(StringView label)
		{
			vk::DebugUtilsLabelEXT info;
			info.pLabelName = label.str.data();
			cmd->beginDebugUtilsLabelEXT(info);
		}

		void CommandEncoderImpl::popDebugGroup()
		{
			cmd->endDebugUtilsLabelEXT();
		}

		CommandBuffer CommandEncoderImpl::finishEncoding()
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Generic);
			currentMode = EncoderModeEnum::Undefined;

			// transition images back to default state
			while (!transitionedImages.empty())
			{
				const Texture t = transitionedImages.begin()->first;
				imageTransition(t, t->defaultState);
			}

			cmd->end();

			CommandBuffer buffer = CommandBuffer(systemMemory().createHolder<CommandBufferImpl>());
			buffer->buffer = std::move(cmd);
			return buffer;
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

			ankerl::svector<vk::RenderingAttachmentInfo, 4> attachments;
			for (const auto &it : desc.colorAttachments)
			{
				vk::RenderingAttachmentInfo info;
				// todo info.clearValue;
				info.imageView = it.view->view;
				info.loadOp = convertLoadOperation(it.loadOp);
				info.storeOp = convertStoreOperation(it.storeOp);
				attachments.push_back(std::move(info));
			}

			vk::RenderingAttachmentInfo depth;
			vk::RenderingAttachmentInfo stencil;
			if (desc.depthStencilAttachment)
			{
				// todo depth.clearValue;
				depth.imageView = desc.depthStencilAttachment->view->view;
				depth.loadOp = convertLoadOperation(desc.depthStencilAttachment->depthLoadOp);
				depth.storeOp = convertStoreOperation(desc.depthStencilAttachment->depthStoreOp);
			}

			vk::RenderingInfo info;
			info.colorAttachmentCount = attachments.size();
			info.pColorAttachments = attachments.data();
			info.pDepthAttachment = &depth;
			info.pStencilAttachment = &stencil;
			info.renderArea = vk::Rect2D(); // todo
			cmd->beginRendering(info);
		}

		void CommandEncoderImpl::endRenderPass()
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			currentMode = EncoderModeEnum::Generic;

			cmd->endRendering();
		}

		void CommandEncoderImpl::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			vk::Rect2D rect = vk::Rect2D({ (sint32)x, (sint32)y }, { w, h });
			cmd->setScissor(0, rect);
		}

		void CommandEncoderImpl::setPipeline(const RenderPipeline &pipeline)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);
			currentPipelineLayout = pipeline->layout->layout;
		}

		void CommandEncoderImpl::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPipelineLayout, binding, group->set.value, dynamicOffsets);
		}

		void CommandEncoderImpl::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->bindVertexBuffers(slot, buffer->buffer.value, offset);
		}

		void CommandEncoderImpl::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->bindIndexBuffer(buffer->buffer, offset, convertIndexFormat(format));
		}

		void CommandEncoderImpl::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void CommandEncoderImpl::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			CAGE_ASSERT(currentMode == EncoderModeEnum::Rendering);
			cmd->draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}
	}
}
