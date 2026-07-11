#include "gpu.h"

namespace cage
{
	namespace gpuImpl
	{
		CommandBuffer::CommandBuffer(const CommandEncoder &commandEncoder)
		{
			// todo
		}

		CommandBuffer::~CommandBuffer()
		{
			// todo
		}

		CommandEncoder::CommandEncoder(const Device &device, const gpu::CommandEncoderDescriptor &desc)
		{
			// todo
		}

		CommandEncoder::~CommandEncoder()
		{
			// todo
		}

		void CommandEncoder::copyTextureToBuffer(const gpu::TexelCopyTextureInfo &source, const gpu::TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			// todo
		}

		RenderPassEncoder::RenderPassEncoder(const CommandEncoder &commandEncoder, const gpu::RenderPassDescriptor &desc)
		{
			// todo
		}

		RenderPassEncoder::~RenderPassEncoder()
		{
			// todo
		}

		void RenderPassEncoder::pushDebugGroup(gpu::StringView label)
		{
			// todo
		}

		void RenderPassEncoder::popDebugGroup()
		{
			// todo
		}

		void RenderPassEncoder::endPass()
		{
			// todo
		}

		void RenderPassEncoder::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			// todo
		}

		void RenderPassEncoder::setPipeline(const gpu::RenderPipeline &pipeline)
		{
			// todo
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const gpu::BindGroup &group)
		{
			// todo
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const gpu::BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			// todo
		}

		void RenderPassEncoder::setVertexBuffer(uint32 slot, const gpu::Buffer &buffer, uint64 offset, uint64 size)
		{
			// todo
		}

		void RenderPassEncoder::setIndexBuffer(const gpu::Buffer &buffer, gpu::IndexFormatEnum format, uint64 offset, uint64 size)
		{
			// todo
		}

		void RenderPassEncoder::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			// todo
		}

		void RenderPassEncoder::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			// todo
		}
	}
}
