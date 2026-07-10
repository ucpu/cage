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

		uint64 Buffer::GetSize() const
		{
			return 0; // todo
		}

		BufferUsage Buffer::GetUsage() const
		{
			return {}; // todo
		}

		PointerRange<char> Buffer::GetMappedRange()
		{
			return {}; // todo
		}

		void Buffer::Unmap()
		{
			// todo
		}

		TextureFormat Texture::GetFormat() const
		{
			return TextureFormat::Undefined; // todo
		}

		TextureView Texture::CreateView(const TextureViewDescriptor &desc)
		{
			return TextureView(systemMemory().createHolder<gpuImpl::TextureView>(*Get(), desc));
		}

		uint32 Texture::GetWidth() const
		{
			return 0; // todo
		}

		uint32 Texture::GetHeight() const
		{
			return 0; // todo
		}

		uint32 Texture::GetDepthOrArrayLayers() const
		{
			return 0; // todo
		}

		uint32 Texture::GetMipLevelCount() const
		{
			return 0; // todo
		}

		TextureDimension Texture::GetDimension() const
		{
			return TextureDimension::Undefined; // todo
		}

		RenderPassEncoder CommandEncoder::BeginRenderPass(const RenderPassDescriptor &desc)
		{
			return RenderPassEncoder(systemMemory().createHolder<gpuImpl::RenderPassEncoder>(*Get(), desc));
		}

		CommandBuffer CommandEncoder::Finish()
		{
			return CommandBuffer(systemMemory().createHolder<gpuImpl::CommandBuffer>(*Get()));
		}

		void CommandEncoder::CopyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize)
		{
			// todo
		}

		void RenderPassEncoder::PushDebugGroup(StringView label)
		{
			// todo
		}

		void RenderPassEncoder::PopDebugGroup()
		{
			// todo
		}

		void RenderPassEncoder::End()
		{
			// todo
		}

		void RenderPassEncoder::SetScissorRect(uint32 x, uint32 y, uint32 w, uint32 h)
		{
			// todo
		}

		void RenderPassEncoder::SetPipeline(const RenderPipeline &pipeline)
		{
			// todo
		}

		void RenderPassEncoder::SetBindGroup(uint32 binding, const BindGroup &group)
		{
			// todo
		}

		void RenderPassEncoder::SetBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets)
		{
			// todo
		}

		void RenderPassEncoder::SetVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size)
		{
			// todo
		}

		void RenderPassEncoder::SetIndexBuffer(const Buffer &buffer, IndexFormat format, uint64 offset, uint64 size)
		{
			// todo
		}

		void RenderPassEncoder::DrawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance)
		{
			// todo
		}

		void RenderPassEncoder::Draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance)
		{
			// todo
		}

		Buffer Device::CreateBuffer(const BufferDescriptor &desc)
		{
			return Buffer(systemMemory().createHolder<gpuImpl::Buffer>(desc));
		}

		Texture Device::CreateTexture(const TextureDescriptor &desc)
		{
			return Texture(systemMemory().createHolder<gpuImpl::Texture>(desc));
		}

		Sampler Device::CreateSampler(const SamplerDescriptor &desc)
		{
			return Sampler(systemMemory().createHolder<gpuImpl::Sampler>(desc));
		}

		BindGroupLayout Device::CreateBindGroupLayout(const BindGroupLayoutDescriptor &desc)
		{
			return BindGroupLayout(systemMemory().createHolder<gpuImpl::BindGroupLayout>(desc));
		}

		BindGroup Device::CreateBindGroup(const BindGroupDescriptor &desc)
		{
			return BindGroup(systemMemory().createHolder<gpuImpl::BindGroup>(desc));
		}

		CommandEncoder Device::CreateCommandEncoder(const CommandEncoderDescriptor &desc)
		{
			return CommandEncoder(systemMemory().createHolder<gpuImpl::CommandEncoder>(desc));
		}

		ShaderModule Device::CreateShaderModule(const ShaderModuleDescriptor &desc)
		{
			return ShaderModule(systemMemory().createHolder<gpuImpl::ShaderModule>(desc));
		}

		PipelineLayout Device::CreatePipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			return PipelineLayout(systemMemory().createHolder<gpuImpl::PipelineLayout>(desc));
		}

		void Device::WriteBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			// todo
		}

		void Device::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			// todo
		}

		void Device::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			return WriteTexture(dest, data.cast<const char>(), layout, extents);
		}
	}
}
