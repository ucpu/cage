#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		CommandBufferImpl::CommandBufferImpl(vk::Device device) : device(device) {}

		CommandBufferImpl::~CommandBufferImpl() {}

		vk::UniqueCommandBuffer CommandBufferImpl::newBuffer()
		{
			thread_local vk::UniqueCommandPool pool;
			if (!pool)
			{
				vk::CommandPoolCreateInfo info;
				pool = device.createCommandPoolUnique(info);
			}

			vk::CommandBufferAllocateInfo info;
			info.commandBufferCount = 1;
			info.commandPool = *pool;
			return std::move(device.allocateCommandBuffersUnique(info)[0]);
		}

		CommandEncoderImpl::CommandEncoderImpl(const DeviceImpl &device, const CommandEncoderDescriptor &desc)
		{
			buffer = CommandBuffer(systemMemory().createHolder<CommandBufferImpl>(device.device));
			cmd = buffer->newBuffer();

			vk::CommandBufferBeginInfo info;
			info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			cmd->begin(info);
		}

		CommandEncoderImpl::~CommandEncoderImpl() {}

		void CommandEncoderImpl::copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			// todo
		}

		CommandBuffer CommandEncoderImpl::finishEncoding()
		{
			cmd->end();
			buffer->buffers.push_back(std::move(cmd));
			return std::move(buffer);
		}

		RenderPassEncoderImpl::RenderPassEncoderImpl(const CommandEncoder &commandEncoder, const RenderPassDescriptor &desc)
		{
			encoder = commandEncoder;

			ankerl::svector<vk::RenderingAttachmentInfo, 4> attachments;
			for (const auto &it : desc.colorAttachments)
			{
				vk::RenderingAttachmentInfo info;
				// todo info.clearValue;
				info.imageView = *it.view->view;
				info.loadOp = convertLoadOperation(it.loadOp);
				info.storeOp = convertStoreOperation(it.storeOp);
				attachments.push_back(std::move(info));
			}

			vk::RenderingAttachmentInfo depth;
			vk::RenderingAttachmentInfo stencil;
			if (desc.depthStencilAttachment)
			{
				// todo depth.clearValue;
				depth.imageView = *desc.depthStencilAttachment->view->view;
				depth.loadOp = convertLoadOperation(desc.depthStencilAttachment->depthLoadOp);
				depth.storeOp = convertStoreOperation(desc.depthStencilAttachment->depthStoreOp);
			}

			vk::RenderingInfo info;
			info.colorAttachmentCount = attachments.size();
			info.pColorAttachments = attachments.data();
			info.pDepthAttachment = &depth;
			info.pStencilAttachment = &stencil;
			info.renderArea = vk::Rect2D(); // todo
			encoder->cmd->beginRendering(info);
		}

		RenderPassEncoderImpl::~RenderPassEncoderImpl() {}

		void RenderPassEncoderImpl::pushDebugGroup(StringView label)
		{
			vk::DebugMarkerMarkerInfoEXT info;
			info.pMarkerName = label.str.data();
			encoder->cmd->debugMarkerBeginEXT(info);
		}

		void RenderPassEncoderImpl::popDebugGroup()
		{
			encoder->cmd->debugMarkerEndEXT();
		}

		void RenderPassEncoderImpl::endPass()
		{
			encoder->cmd->endRendering();
		}

		void RenderPassEncoderImpl::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			vk::Rect2D rect = vk::Rect2D({ (sint32)x, (sint32)y }, { w, h });
			encoder->cmd->setScissor(0, rect);
		}

		void RenderPassEncoderImpl::setPipeline(const RenderPipeline &pipeline)
		{
			encoder->cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->pipeline);
			layout = *pipeline->layout->layout;
		}

		void RenderPassEncoderImpl::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			encoder->cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, binding, *group->set, dynamicOffsets);
		}

		void RenderPassEncoderImpl::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			encoder->cmd->bindVertexBuffers(slot, *buffer->buffer, offset);
		}

		void RenderPassEncoderImpl::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
		{
			encoder->cmd->bindIndexBuffer(*buffer->buffer, offset, convertIndexFormat(format));
		}

		void RenderPassEncoderImpl::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			encoder->cmd->drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void RenderPassEncoderImpl::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			encoder->cmd->draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}
	}
}
