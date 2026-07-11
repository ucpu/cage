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
			return get()->mappedRange;
		}

		void Buffer::unmap()
		{
			get()->unmap();
		}

		RenderPassEncoder CommandEncoder::beginRenderPass(const RenderPassDescriptor &desc)
		{
			return RenderPassEncoder(systemMemory().createHolder<RenderPassEncoderImpl>(*this, desc));
		}

		CommandBuffer CommandEncoder::finishEncoding()
		{
			return get()->finishEncoding();
		}

		void CommandEncoder::copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			get()->copyTextureToBuffer(source, destination, copySize);
		}

		Buffer Device::createBuffer(const BufferDescriptor &desc)
		{
			return Buffer(systemMemory().createHolder<BufferImpl>(*get(), desc));
		}

		Texture Device::createTexture(const TextureDescriptor &desc)
		{
			return Texture(systemMemory().createHolder<TextureImpl>(*get(), desc));
		}

		Sampler Device::createSampler(const SamplerDescriptor &desc)
		{
			return Sampler(systemMemory().createHolder<SamplerImpl>(*get(), desc));
		}

		BindGroupLayout Device::createBindGroupLayout(const BindGroupLayoutDescriptor &desc)
		{
			return BindGroupLayout(systemMemory().createHolder<BindGroupLayoutImpl>(*get(), desc));
		}

		BindGroup Device::createBindGroup(const BindGroupDescriptor &desc)
		{
			return BindGroup(systemMemory().createHolder<BindGroupImpl>(*get(), desc));
		}

		CommandEncoder Device::createCommandEncoder(const CommandEncoderDescriptor &desc)
		{
			return CommandEncoder(systemMemory().createHolder<CommandEncoderImpl>(*get(), desc));
		}

		ShaderModule Device::createShaderModule(const ShaderModuleDescriptor &desc)
		{
			return ShaderModule(systemMemory().createHolder<ShaderModuleImpl>(*get(), desc));
		}

		PipelineLayout Device::createPipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			return PipelineLayout(systemMemory().createHolder<PipelineLayoutImpl>(*get(), desc));
		}

		RenderPipeline Device::createRenderPipeline(const RenderPipelineDescriptor &desc)
		{
			return RenderPipeline(systemMemory().createHolder<RenderPipelineImpl>(*get(), desc));
		}

		void Device::writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			get()->writeBuffer(buffer, offset, data);
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			get()->writeTexture(dest, data, layout, extents);
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			return writeTexture(dest, data.cast<const char>(), layout, extents);
		}

		Texture Device::windowAcquireTexture(Window *window)
		{
			return get()->acquireWindowSurfaceTexture(window);
		}

		void Device::windowPresent(Window *window)
		{
			get()->windowPresent(window);
		}

		void Device::windowWaitFence(Window *window)
		{
			get()->windowWaitFence(window);
		}

		void RenderPassEncoder::pushDebugGroup(StringView label)
		{
			get()->pushDebugGroup(label);
		}

		void RenderPassEncoder::popDebugGroup()
		{
			get()->popDebugGroup();
		}

		void RenderPassEncoder::endPass()
		{
			get()->endPass();
		}

		void RenderPassEncoder::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			get()->setScissorRect(x, y, w, h);
		}

		void RenderPassEncoder::setPipeline(const RenderPipeline &pipeline)
		{
			get()->setPipeline(pipeline);
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const BindGroup &group)
		{
			get()->setBindGroup(binding, group, {});
		}

		void RenderPassEncoder::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			get()->setBindGroup(binding, group, dynamicOffsets);
		}

		void RenderPassEncoder::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			get()->setVertexBuffer(slot, buffer, offset, size);
		}

		void RenderPassEncoder::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
		{
			get()->setIndexBuffer(buffer, format, offset, size);
		}

		void RenderPassEncoder::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			get()->drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void RenderPassEncoder::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			get()->draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}

		TextureFormatEnum Texture::getFormat() const
		{
			return get()->format;
		}

		TextureView Texture::createView(const TextureViewDescriptor &desc)
		{
			return TextureView(systemMemory().createHolder<TextureViewImpl>(*get(), desc));
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
