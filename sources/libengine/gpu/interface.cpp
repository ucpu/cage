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

		void Buffer::flush()
		{
			ScopeLock lock(get()->buffer.device->mutex);
			get()->flush();
		}

		void Buffer::invalidate()
		{
			ScopeLock lock(get()->buffer.device->mutex);
			get()->invalidate();
		}

		EncoderModeEnum CommandEncoder::mode() const
		{
			return get()->currentMode;
		}

		void CommandEncoder::pushDebugGroup(StringView label)
		{
			get()->pushDebugGroup(label);
		}

		void CommandEncoder::popDebugGroup()
		{
			get()->popDebugGroup();
		}

		CommandBuffer CommandEncoder::finishEncoding()
		{
			ScopeLock lock(get()->cmd.device->mutex);
			return get()->finishEncoding();
		}

		void CommandEncoder::copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			get()->copyTextureToBuffer(source, destination, copySize);
		}

		void CommandEncoder::beginRenderPass(const RenderPassDescriptor &desc)
		{
			get()->beginRenderPass(desc);
		}

		void CommandEncoder::endRenderPass()
		{
			get()->endRenderPass();
		}

		void CommandEncoder::setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			get()->setScissorRect(x, y, w, h);
		}

		void CommandEncoder::setPipeline(const RenderPipeline &pipeline)
		{
			get()->setPipeline(pipeline);
		}

		void CommandEncoder::setBindGroup(uint32 binding, const BindGroup &group)
		{
			get()->setBindGroup(binding, group, {});
		}

		void CommandEncoder::setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			get()->setBindGroup(binding, group, dynamicOffsets);
		}

		void CommandEncoder::setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			get()->setVertexBuffer(slot, buffer, offset, size);
		}

		void CommandEncoder::setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size)
		{
			get()->setIndexBuffer(buffer, format, offset, size);
		}

		void CommandEncoder::drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			get()->drawIndexed(indicesCount, instancesCount, firstIndex, baseVertex, firstInstance);
		}

		void CommandEncoder::draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			get()->draw(verticesCount, instancesCount, firstVertex, firstInstance);
		}

		Buffer Device::createBuffer(const BufferDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return Buffer(systemMemory().createHolder<BufferImpl>(*get(), desc));
		}

		Texture Device::createTexture(const TextureDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return Texture(systemMemory().createHolder<TextureImpl>(*get(), desc));
		}

		Sampler Device::createSampler(const SamplerDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return Sampler(systemMemory().createHolder<SamplerImpl>(*get(), desc));
		}

		BindGroupLayout Device::createBindGroupLayout(const BindGroupLayoutDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return BindGroupLayout(systemMemory().createHolder<BindGroupLayoutImpl>(*get(), desc));
		}

		BindGroup Device::createBindGroup(const BindGroupDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return BindGroup(systemMemory().createHolder<BindGroupImpl>(*get(), desc));
		}

		CommandEncoder Device::createCommandEncoder(const CommandEncoderDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return CommandEncoder(systemMemory().createHolder<CommandEncoderImpl>(*get(), desc));
		}

		ShaderModule Device::createShaderModule(const ShaderModuleDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return ShaderModule(systemMemory().createHolder<ShaderModuleImpl>(*get(), desc));
		}

		PipelineLayout Device::createPipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return PipelineLayout(systemMemory().createHolder<PipelineLayoutImpl>(*get(), desc));
		}

		RenderPipeline Device::createRenderPipeline(const RenderPipelineDescriptor &desc)
		{
			ScopeLock lock(get()->mutex);
			return RenderPipeline(systemMemory().createHolder<RenderPipelineImpl>(*get(), desc));
		}

		void Device::writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			ScopeLock lock(get()->mutex);
			get()->writeBuffer(buffer, offset, data);
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			ScopeLock lock(get()->mutex);
			get()->writeTexture(dest, data, layout, extents);
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			// no lock
			return writeTexture(dest, data.cast<const char>(), layout, extents);
		}

		void Device::tick()
		{
			ScopeLock lock(get()->mutex);
			get()->tick();
		}

		void Device::wait(const Future &future)
		{
			// no lock
			get()->wait(future);
		}

		void Device::submitAndPresentWindows(PointerRange<const CommandBuffer> buffers, PointerRange<WindowPresentationDescriptor> windows)
		{
			ScopeLock lock(get()->mutex);
			get()->submitAndPresentWindows(buffers, windows);
		}

		TextureView Texture::createView()
		{
			// no lock
			return createView({});
		}

		TextureView Texture::createView(const TextureViewDescriptor &desc)
		{
			ScopeLock lock(get()->image.device->mutex);
			return TextureView(systemMemory().createHolder<TextureViewImpl>(*this, desc));
		}

		Vec3i Texture::getResolution() const
		{
			return get()->resolution;
		}

		uint32 Texture::getArrayLayers() const
		{
			return get()->arrayLayers;
		}

		uint32 Texture::getMipLevels() const
		{
			return get()->mipLevels;
		}

		TextureDimensionEnum Texture::getDimension() const
		{
			return get()->dimension;
		}

		TextureFormatEnum Texture::getFormat() const
		{
			return get()->format;
		}

		TextureUsageFlags Texture::getUsage() const
		{
			return get()->usage;
		}

		Texture TextureView::getTexture() const
		{
			return get()->texture;
		}
	}
}
