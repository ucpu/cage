#include "gpu.h"

#include <cage-core/tasks.h>

namespace cage
{
	namespace gpu
	{
		uint64 Buffer::getSize() const
		{
			return get()->size;
		}

		BufferUsageFlags Buffer::getUsage() const
		{
			return get()->usage;
		}

		PointerRange<char> Buffer::getMappedRange() const
		{
			return get()->mappedRange;
		}

		void Buffer::flush()
		{
			get()->flush();
		}

		void Buffer::invalidate()
		{
			get()->invalidate();
		}

		EncoderModeEnum CommandEncoder::mode() const
		{
			return get()->currentMode;
		}

		void CommandEncoder::pushDebugGroup(const AssetLabel &label)
		{
			get()->pushDebugGroup(label);
		}

		void CommandEncoder::popDebugGroup()
		{
			get()->popDebugGroup();
		}

		CommandBuffer CommandEncoder::finishEncoding()
		{
			return get()->finishEncoding();
		}

		void CommandEncoder::copyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size)
		{
			get()->copyBufferToBuffer(source, sourceOffset, destination, destinationOffset, size);
		}

		void CommandEncoder::copyBufferToTexture(const Buffer &source, uint64 sourceOffset, const TexelCopyTextureInfo &destination, Vec3i copySize)
		{
			get()->copyBufferToTexture(source, sourceOffset, destination, copySize);
		}

		void CommandEncoder::copyTextureToBuffer(const TexelCopyTextureInfo &source, const Buffer &destination, uint64 destinationOffset, Vec3i copySize)
		{
			get()->copyTextureToBuffer(source, destination, destinationOffset, copySize);
		}

		void CommandEncoder::resolveQuerySet(const QuerySet &querySet, uint32 firstQuery, uint32 queryCount, const Buffer &destination, uint64 destinationOffset)
		{
			get()->resolveQuerySet(querySet, firstQuery, queryCount, destination, destinationOffset);
		}

		void CommandEncoder::writeTimestamp(const QuerySet &querySet, uint32 queryIndex)
		{
			get()->writeTimestamp(querySet, queryIndex);
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

		void CommandEncoder::setViewport(Real x, Real y, Real width, Real height, Real minDepth, Real maxDepth)
		{
			get()->setViewport(x, y, width, height, minDepth, maxDepth);
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
			Buffer b = Buffer(systemMemory().createHolder<BufferImpl>(*get(), desc));
			b->defaultState = BufferStateEnum::Read;
			return b;
		}

		Texture Device::createTexture(const TextureDescriptor &desc)
		{
			Texture t = Texture(systemMemory().createHolder<TextureImpl>(*get(), desc));

			{ // initial image layout transition
				// transition from undefined directly to sampled is forbidden
				ImageStateEnum intermediate = ImageStateEnum::TransferDst;
				if (any(desc.usage & TextureUsageFlags::RenderAttachment))
				{
					if (convertAspectMask(desc.format) == vk::ImageAspectFlagBits::eColor)
						intermediate = ImageStateEnum::ColorAttachment;
					else
						intermediate = ImageStateEnum::DepthAttachment;
				}
				ScopeLock lock(get()->mutex);
				CommandEncoderImpl &enc = get()->addCommands();
				enc.imageTransitionPermanent(t, ImageStateEnum::Undefined, intermediate);
				enc.imageTransitionPermanent(t, intermediate, ImageStateEnum::Sampled);
			}

			return t;
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
			ScopeLock lock(get()->mutex); // uses shared descriptorPool
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

		QuerySet Device::createQuerySet(const QuerySetDescriptor &desc)
		{
			return QuerySet(systemMemory().createHolder<QuerySetImpl>(*get(), desc));
		}

		RenderPipeline Device::createRenderPipeline(const RenderPipelineDescriptor &desc)
		{
			return RenderPipeline(systemMemory().createHolder<RenderPipelineImpl>(*get(), desc));
		}

		namespace
		{
			struct RenderPipelineTask : private Immovable
			{
				Device device;
				RenderPipelineDescriptor desc;
				std::function<void(StatusEnum, RenderPipeline)> callback;

				void operator()(uint32)
				{
					RenderPipeline rp;
					try
					{
						rp = device.createRenderPipeline(desc);
					}
					catch (...)
					{
						callback(StatusEnum::Error, {});
						return;
					}
					callback(StatusEnum::Success, std::move(rp));
				}
			};
		}

		void Device::createRenderPipelineAsyncTypeErased(const RenderPipelineDescriptor &descriptor, std::function<void(StatusEnum, RenderPipeline)> callback)
		{
			Holder<RenderPipelineTask> data = systemMemory().createHolder<RenderPipelineTask>();
			data->device = *this;
			data->desc = descriptor; // make a copy
			data->callback = std::move(callback);
			Holder<AsyncTask> t = tasksRunAsync<RenderPipelineTask>("render pipeline async", std::move(data));
			ScopeLock lock(get()->mutex);
			get()->disposingTasks.push_back(std::move(t));
		}

		void Device::writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data)
		{
			CAGE_ASSERT(buffer.getSize() >= offset + data.size());
			BufferDescriptor desc;
			desc.label = "staging buffer";
			desc.size = data.size();
			desc.usage = BufferUsageFlags::MapWrite | BufferUsageFlags::CopySrc;
			Buffer staging = createBuffer(desc);
			CAGE_ASSERT(staging.getMappedRange().size() >= data.size());
			detail::memcpy(staging.getMappedRange().data(), data.data(), data.size());
			ScopeLock lock(get()->mutex);
			CommandEncoderImpl &enc = get()->addCommands();
			enc.copyBufferToBuffer(staging, 0, buffer, offset, data.size());
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, Vec3i extents)
		{
			BufferDescriptor desc;
			desc.label = "staging buffer";
			desc.size = data.size();
			desc.usage = BufferUsageFlags::MapWrite | BufferUsageFlags::CopySrc;
			Buffer staging = createBuffer(desc);
			CAGE_ASSERT(staging.getMappedRange().size() >= data.size());
			detail::memcpy(staging.getMappedRange().data(), data.data(), data.size());
			ScopeLock lock(get()->mutex);
			CommandEncoderImpl &enc = get()->addCommands();
			enc.copyBufferToTexture(staging, 0, dest, extents);
		}

		void Device::writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, Vec3i extents)
		{
			return writeTexture(dest, data.cast<const char>(), extents);
		}

		void Device::setVsyncPreference(bool vsync, bool tripleBuffer)
		{
			ScopeLock lock(get()->mutex);
			get()->setVsyncPreference(vsync, tripleBuffer);
		}

		double Device::getTimestampsConversion() const
		{
			return get()->getTimestampsConversion();
		}

		void Device::submitAndPresent(PointerRange<const CommandBuffer> buffers, PointerRange<WindowPresentationDescriptor> windows)
		{
			// locking is inside
			get()->submitAndPresent(buffers, windows);
		}

		void Device::submit(PointerRange<const CommandBuffer> buffers)
		{
			ScopeLock lock(get()->mutex);
			get()->submit(buffers);
		}

		void Device::waitDeviceIdle()
		{
			ScopeLock lock(get()->mutex);
			get()->waitDeviceIdle();
		}

		TextureView Texture::createView(const TextureViewDescriptor &desc)
		{
			return TextureView(systemMemory().createHolder<TextureViewImpl>(*this, desc));
		}

		Vec3i Texture::getResolution() const
		{
			return get()->resolution;
		}

		uint32 Texture::getArrayLayersCount() const
		{
			return get()->arrayLayersCount;
		}

		uint32 Texture::getMipLevelsCount() const
		{
			return get()->mipLevelsCount;
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

		uint32 TextureView::getArrayLayersOffset() const
		{
			return get()->arrayLayersOffset;
		}

		uint32 TextureView::getArrayLayersCount() const
		{
			return get()->arrayLayersCount;
		}

		uint32 TextureView::getMipLevelsOffset() const
		{
			return get()->mipLevelsOffset;
		}

		uint32 TextureView::getMipLevelsCount() const
		{
			return get()->mipLevelsCount;
		}

		TextureDimensionEnum TextureView::getDimension() const
		{
			return get()->dimension;
		}

		Vec2i RenderPassDescriptor::targetResolution() const
		{
			Vec2i res;
			const auto &update = [&](const TextureView &v)
			{
				Vec2i r = Vec2i(v->texture.getResolution());
				for (uint32 i = 0; i < v->mipLevelsOffset; i++)
					r /= 2;
				if (res == Vec2i())
					res = r;
				else
					res = min(res, r);
			};
			for (const auto &it : colorAttachments)
			{
				update(it.view);
			}
			if (depthStencilAttachment)
			{
				update(depthStencilAttachment->view);
			}
			return res;
		}
	}
}
