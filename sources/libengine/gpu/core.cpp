#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuInterface.h>

namespace cage
{
	namespace gpu
	{
		StringView::StringView() {}

		StringView::StringView(const char *ptr)
		{
			// todo
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
			return {}; // todo
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
			return {}; // todo
		}

		CommandBuffer CommandEncoder::Finish()
		{
			return {}; // todo
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
			return {}; // todo
		}

		Texture Device::CreateTexture(const TextureDescriptor &desc)
		{
			return {}; // todo
		}

		Sampler Device::CreateSampler(const SamplerDescriptor &desc)
		{
			return {}; // todo
		}

		BindGroupLayout Device::CreateBindGroupLayout(const BindGroupLayoutDescriptor &desc)
		{
			return {}; // todo
		}

		BindGroup Device::CreateBindGroup(const BindGroupDescriptor &desc)
		{
			return {}; // todo
		}

		CommandEncoder Device::CreateCommandEncoder(const CommandEncoderDescriptor &desc)
		{
			return {}; // todo
		}

		ShaderModule Device::CreateShaderModule(const ShaderModuleDescriptor &desc)
		{
			return {}; // todo
		}

		PipelineLayout Device::CreatePipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			return {}; // todo
		}

		void Queue::WriteBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			// todo
		}

		void Queue::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			// todo
		}

		void Queue::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents)
		{
			return WriteTexture(dest, data.cast<const char>(), layout, extents);
		}
	}
}
