#include <cage-core/profiling.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/graphicsPipeline.h>
#include <cage-engine/model.h>
#include <cage-engine/shader.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace privat
	{
		GraphicsBindings getEmptyBindings(GraphicsDevice *device);
	}

	namespace
	{
		struct PassData : public RenderPassConfig
		{
			gpu::RenderPipeline pipeline;
		};

		class GraphicsEncoderImpl : public GraphicsEncoder
		{
		public:
			GraphicsDevice *device = nullptr;
			gpu::CommandEncoder encoder;
			PassData passData;
			GraphicsFrameStatistics statistics;

			GraphicsEncoderImpl(GraphicsDevice *device, const AssetLabel &label_) : device(device)
			{
				this->label = label_;
				CAGE_ASSERT(device);
			}

			void nextPass(const RenderPassConfig &config)
			{
				const ProfilingScope profiling("encoder next pass");
				if (!encoder)
					encoder = device->nativeDevice()->createCommandEncoder({ label });
				else if (encoder.mode() == gpu::EncoderModeEnum::Rendering)
				{
					encoder.endRenderPass();
					passData = {};
				}
				{
					passData = PassData{ config };
					if (!passData.bindings)
						passData.bindings = privat::getEmptyBindings(device);

					Vec2i res;
					gpu::RenderPassDescriptor rpd;
					rpd.label = label;

					rpd.colorAttachments.reserve(passData.colorTargets.size());
					for (const auto &it : passData.colorTargets)
					{
						gpu::RenderPassDescriptor::ColorAttachment rpca;
						rpca.clearValue = it.clearValue;
						rpca.loadOp = it.clear ? gpu::LoadOpEnum::Clear : gpu::LoadOpEnum::Load;
						rpca.storeOp = gpu::StoreOpEnum::Store;
						CAGE_ASSERT(it.texture->nativeView());
						rpca.view = it.texture->nativeView();
						rpd.colorAttachments.push_back(std::move(rpca));
						res = it.texture->resolution();
					}

					if (passData.depthTarget)
					{
						CAGE_ASSERT(passData.depthTarget->texture->nativeView());
						gpu::RenderPassDescriptor::DepthStencilAttachment rpdsa;
						rpdsa.view = passData.depthTarget->texture->nativeView();
						rpdsa.depthLoadOp = passData.depthTarget->clear ? gpu::LoadOpEnum::Clear : gpu::LoadOpEnum::Load;
						rpdsa.depthStoreOp = gpu::StoreOpEnum::Store;
						rpdsa.depthClearValue = 1;
						rpd.depthStencilAttachment = std::move(rpdsa);
						res = passData.depthTarget->texture->resolution();
					}

					encoder.beginRenderPass(rpd);
					encoder.setViewport(0, 0, res[0], res[1]);
					encoder.setScissorRect(0, 0, res[0], res[1]);
				}
				statistics.passes++;
			}

			void scissors(Vec2i origin, Vec2i size)
			{
				CAGE_ASSERT(encoder && encoder.mode() == gpu::EncoderModeEnum::Rendering);
				encoder.setScissorRect(origin[0], origin[1], size[0], size[1]);
			}

			void draw(DrawConfig config)
			{
				CAGE_ASSERT(encoder && encoder.mode() == gpu::EncoderModeEnum::Rendering);
				CAGE_ASSERT(config.shader);
				CAGE_ASSERT(config.model);
				if (!config.material)
					config.material = newGraphicsBindings(device, nullptr, config.model).first;
				if (!config.bindings)
					config.bindings = privat::getEmptyBindings(device);

				gpu::RenderPipeline pip = newGraphicsPipeline(device, convertPipelineConfig(passData, config));
				if (!pip)
					return; // pipeline not ready

				if (!passData.pipeline || passData.pipeline.get() != pip.get())
				{
					encoder.setPipeline(pip);
					encoder.setBindGroup(0, passData.bindings.group);
					passData.pipeline = pip;
					statistics.pipelineSwitches++;
				}

				encoder.setBindGroup(1, config.material.group);
				CAGE_ASSERT(config.bindings.dynamicBuffersCount <= config.dynamicOffsets.size());
				encoder.setBindGroup(2, config.bindings.group, PointerRange<const uint32>(config.dynamicOffsets).subRange(0, config.bindings.dynamicBuffersCount));

				encoder.setVertexBuffer(0, config.model->geometryBuffer->nativeBuffer());
				if (config.model->indicesCount)
					encoder.setIndexBuffer(config.model->geometryBuffer->nativeBuffer(), gpu::IndexFormatEnum::Uint32, config.model->indicesOffset);

				if (config.model->indicesCount)
					encoder.drawIndexed(config.model->indicesCount, config.instances);
				else
					encoder.draw(config.model->verticesCount, config.instances);

				statistics.drawCalls++;
				statistics.primitives += config.model->primitivesCount * config.instances;
			}

			void submit()
			{
				const ProfilingScope profiling("encoder submit");
				CAGE_ASSERT(encoder);
				if (encoder.mode() == gpu::EncoderModeEnum::Rendering)
					encoder.endRenderPass();
				gpu::CommandBuffer b = encoder.finishEncoding();
				encoder = {};
				device->insertCommandBuffer(std::move(b), statistics);
				statistics = {};
			}

			void pushScope(StringPointer name)
			{
				if (!encoder)
					encoder = device->nativeDevice()->createCommandEncoder({ label });
				encoder.pushDebugGroup((const char *)name);
			}

			void popScope()
			{
				CAGE_ASSERT(encoder);
				encoder.popDebugGroup();
			}
		};
	}

	namespace detail
	{
		RenderEncoderNamedScope::RenderEncoderNamedScope(GraphicsEncoder *queue, StringPointer name) : queue(queue)
		{
			GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)queue;
			impl->pushScope(name);
		}

		RenderEncoderNamedScope::~RenderEncoderNamedScope()
		{
			GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)queue;
			impl->popScope();
		}
	}

	detail::RenderEncoderNamedScope GraphicsEncoder::namedScope(StringPointer name)
	{
		return detail::RenderEncoderNamedScope(this, name);
	}

	void GraphicsEncoder::nextPass(const RenderPassConfig &config)
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		impl->nextPass(config);
	}

	void GraphicsEncoder::scissors(Vec2i origin, Vec2i size)
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		impl->scissors(origin, size);
	}

	void GraphicsEncoder::draw(const DrawConfig &config)
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		impl->draw(config);
	}

	void GraphicsEncoder::submit()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		impl->submit();
	}

	GraphicsDevice *GraphicsEncoder::getDevice() const
	{
		const GraphicsEncoderImpl *impl = (const GraphicsEncoderImpl *)this;
		return impl->device;
	}

	const RenderPassConfig &GraphicsEncoder::getCurrentPass() const
	{
		const GraphicsEncoderImpl *impl = (const GraphicsEncoderImpl *)this;
		return (const RenderPassConfig &)impl->passData;
	}

	gpu::CommandEncoder &GraphicsEncoder::nativeEncoder()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		return impl->encoder;
	}

	Holder<GraphicsEncoder> newGraphicsEncoder(GraphicsDevice *device, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsEncoder, GraphicsEncoderImpl>(device, label);
	}
}
