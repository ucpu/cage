#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuCore.h>

namespace cage
{
	namespace gpu
	{
		StringView::StringView() {}

		StringView::StringView(const char *ptr)
		{
			// todo
		}

		uintPtr Buffer::GetSize() const
		{
			return 0; // todo
		}

		void *Buffer::GetConstMappedRange()
		{
			return 0; // todo
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

		void CommandEncoder::CopyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, const Extent3D &copySize)
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

		void RenderPassEncoder::SetScissorRect(Real x, Real y, Real w, Real h)
		{
			// todo
		}

		void RenderPassEncoder::SetPipeline(RenderPipeline pipeline)
		{
			// todo
		}

		void RenderPassEncoder::SetBindGroup(uint32 binding, BindGroup group)
		{
			// todo
		}

		void RenderPassEncoder::SetBindGroup(uint32 binding, BindGroup group, uint32 dynamicBuffersCount, PointerRange<const uint32> dynamicOffsets)
		{
			// todo
		}

		void RenderPassEncoder::SetVertexBuffer(uint32 index, Buffer buffer)
		{
			// todo
		}

		void RenderPassEncoder::SetIndexBuffer(Buffer buffer, IndexFormat format, uint32 indicesOffset)
		{
			// todo
		}

		void RenderPassEncoder::DrawIndexed(uint32 indicesCount, uint32 instancesCount)
		{
			// todo
		}

		void RenderPassEncoder::Draw(uint32 verticesCount, uint32 instancesCount)
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

		RenderPipelineLayout Device::CreatePipelineLayout(const PipelineLayoutDescriptor &desc)
		{
			return {}; // todo
		}

		void Queue::WriteBuffer(Buffer buffer, uintPtr offset, PointerRange<const char> data)
		{
			// todo
		}

		void Queue::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, const Extent3D &extents)
		{
			// todo
		}

		void Queue::WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, const Extent3D &extents)
		{
			return WriteTexture(dest, data.cast<const char>(), layout, extents);
		}
	}
}
