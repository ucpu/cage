#include "gpu.h"

namespace cage
{
	namespace gpuImpl
	{
		CommandBuffer::CommandBuffer(vk::Device device) : device(device) {}

		CommandBuffer::~CommandBuffer() {}

		vk::UniqueCommandBuffer CommandBuffer::newBuffer()
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

		CommandEncoder::CommandEncoder(const Device &device, const gpu::CommandEncoderDescriptor &desc)
		{
			buffer = gpu::CommandBuffer(systemMemory().createHolder<CommandBuffer>(device.device));
			cmd = buffer->newBuffer();

			vk::CommandBufferBeginInfo info;
			info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			cmd->begin(info);
		}

		CommandEncoder::~CommandEncoder() {}

		void CommandEncoder::copyTextureToBuffer(const gpu::TexelCopyTextureInfo &source, const gpu::TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			// todo
		}

		gpu::CommandBuffer CommandEncoder::finishEncoding()
		{
			cmd->end();
			buffer->buffers.push_back(std::move(cmd));
			return std::move(buffer);
		}

		RenderPassEncoder::RenderPassEncoder(const gpu::CommandEncoder &commandEncoder, const gpu::RenderPassDescriptor &desc)
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

		RenderPassEncoder::~RenderPassEncoder() {}

		void RenderPassEncoder::pushDebugGroup(gpu::StringView label)
		{
			vk::DebugMarkerMarkerInfoEXT info;
			info.pMarkerName = label.str.data();
			encoder->cmd->debugMarkerBeginEXT(info);
		}

		void RenderPassEncoder::popDebugGroup()
		{
			encoder->cmd->debugMarkerEndEXT();
		}

		void RenderPassEncoder::endPass()
		{
			encoder->cmd->endRendering();
		}

		void RenderPassEncoder::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			vk::Rect2D rect = vk::Rect2D({ (sint32)x, (sint32)y }, { w, h });
			encoder->cmd->setScissor(0, rect);
		}

		void RenderPassEncoder::setPipeline(const gpu::RenderPipeline &pipeline)
		{
			encoder->cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->pipeline);
			layout = *pipeline->layout->layout;
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const gpu::BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			encoder->cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, binding, *group->set, dynamicOffsets);
		}

		void RenderPassEncoder::setVertexBuffer(uint32 slot, const gpu::Buffer &buffer, uint64 offset, uint64 size)
		{
			encoder->cmd->bindVertexBuffers(slot, *buffer->buffer, offset);
		}

		void RenderPassEncoder::setIndexBuffer(const gpu::Buffer &buffer, gpu::IndexFormatEnum format, uint64 offset, uint64 size)
		{
			encoder->cmd->bindIndexBuffer(*buffer->buffer, offset, convertIndexFormat(format));
		}

		void RenderPassEncoder::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			encoder->cmd->drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void RenderPassEncoder::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			encoder->cmd->draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}
	}
}
