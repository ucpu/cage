#include <cstring>

#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		StringView::StringView(const char *ptr)
		{
			if (ptr)
				str = PointerRange<const char>(ptr, ptr + std::strlen(ptr));
		}

		uint64 Buffer::getSize() const
		{
			return get()->size;
		}

		BufferUsageFlags Buffer::getUsage() const
		{
			return get()->usage;
		}

		PointerRange<char> Buffer::getMappedRange()
		{
			return {}; // todo
		}

		void Buffer::unmap()
		{
			// todo
		}

		RenderPassEncoder CommandEncoder::beginRenderPass(const RenderPassDescriptor &desc)
		{
			return RenderPassEncoder(systemMemory().createHolder<gpuImpl::RenderPassEncoder>(*get(), desc));
		}

		CommandBuffer CommandEncoder::finishEncoding()
		{
			return CommandBuffer(systemMemory().createHolder<gpuImpl::CommandBuffer>(*get()));
		}

		void CommandEncoder::copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			// todo
		}

		Buffer Device::createBuffer(const BufferDescriptor &desc)
		{
			return Buffer(systemMemory().createHolder<gpuImpl::Buffer>(*get(), desc));
		}

		Texture Device::createTexture(const TextureDescriptor &desc)
		{
			return Texture(systemMemory().createHolder<gpuImpl::Texture>(*get(), desc));
		}

		Sampler Device::createSampler(const SamplerDescriptor &desc)
		{
			return Sampler(systemMemory().createHolder<gpuImpl::Sampler>(*get(), desc));
		}

		BindGroupLayout Device::createBindGroupLayout(const BindGroupLayoutDescriptor &desc)
		{
			return BindGroupLayout(systemMemory().createHolder<gpuImpl::BindGroupLayout>(*get(), desc));
		}

		BindGroup Device::createBindGroup(const BindGroupDescriptor &desc)
		{
			return BindGroup(systemMemory().createHolder<gpuImpl::BindGroup>(*get(), desc));
		}

		CommandEncoder Device::createCommandEncoder(const CommandEncoderDescriptor &desc)
		{
			return CommandEncoder(systemMemory().createHolder<gpuImpl::CommandEncoder>(*get(), desc));
		}

		ShaderModule Device::createShaderModule(const ShaderModuleDescriptor &desc)
		{
			return ShaderModule(systemMemory().createHolder<gpuImpl::ShaderModule>(*get(), desc));
		}

		PipelineLayout Device::createPipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			return PipelineLayout(systemMemory().createHolder<gpuImpl::PipelineLayout>(*get(), desc));
		}

		void Device::writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			// todo
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			// todo
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			return writeTexture(dest, data.cast<const char>(), layout, extents);
		}

		void RenderPassEncoder::pushDebugGroup(StringView label)
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

		void RenderPassEncoder::setPipeline(const RenderPipeline &pipeline)
		{
			// todo
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const BindGroup &group)
		{
			// todo
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			// todo
		}

		void RenderPassEncoder::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			// todo
		}

		void RenderPassEncoder::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
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

		TextureFormatEnum Texture::getFormat() const
		{
			return get()->format;
		}

		TextureView Texture::createView(const TextureViewDescriptor &desc)
		{
			return TextureView(systemMemory().createHolder<gpuImpl::TextureView>(*get(), desc));
		}

		uint32 Texture::getWidth() const
		{
			return get()->resolution[0];
		}

		uint32 Texture::getHeight() const
		{
			return get()->resolution[1];
		}

		uint32 Texture::getDepthOrArrayLayers() const
		{
			return get()->resolution[2];
		}

		uint32 Texture::getMipLevelCount() const
		{
			return get()->mipLevels;
		}

		TextureDimensionEnum Texture::getDimension() const
		{
			return get()->dimension;
		}
	}
}
