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
	namespace
	{
		struct PassData : public RenderPassConfig
		{
			wgpu::RenderPipeline pipeline;
		};

		class GraphicsEncoderImpl : public GraphicsEncoder
		{
		public:
			GraphicsDevice *device = nullptr;
			wgpu::CommandEncoder cmdEnc;
			wgpu::RenderPassEncoder renderEnc;
			PassData passData;
			GraphicsFrameStatistics statistics;

			GraphicsEncoderImpl(GraphicsDevice *device, const AssetLabel &label_) : device(device) { this->label = label_; }

			void nextPass(const RenderPassConfig &config)
			{
				const ProfilingScope profiling("encoder next pass");
				if (renderEnc)
				{
					renderEnc.End();
					renderEnc = nullptr;
					passData = {};
				}
				if (!cmdEnc)
				{
					wgpu::CommandEncoderDescriptor desc = {};
					desc.label = label.c_str();
					cmdEnc = device->nativeDevice()->CreateCommandEncoder(&desc);
				}
				{
					passData = PassData{ config };
					if (!passData.bindings)
						passData.bindings = getEmptyBindings(device);

					ankerl::svector<wgpu::RenderPassColorAttachment, 1> atts;
					atts.reserve(passData.colorTargets.size());
					for (const auto &it : passData.colorTargets)
					{
						wgpu::RenderPassColorAttachment rpca = {};
						rpca.clearValue = { it.clearValue[0].value, it.clearValue[1].value, it.clearValue[2].value, it.clearValue[3].value };
						rpca.loadOp = it.clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
						rpca.storeOp = wgpu::StoreOp::Store;
						CAGE_ASSERT(it.texture->nativeView());
						rpca.view = it.texture->nativeView();
						atts.push_back(std::move(rpca));
					}

					wgpu::RenderPassDepthStencilAttachment rpdsa = {};
					if (passData.depthTarget)
					{
						CAGE_ASSERT(passData.depthTarget->texture->nativeView());
						rpdsa.view = passData.depthTarget->texture->nativeView();
						rpdsa.depthLoadOp = passData.depthTarget->clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
						rpdsa.depthStoreOp = wgpu::StoreOp::Store;
						rpdsa.depthClearValue = 1;
					}

					wgpu::RenderPassDescriptor rpd = {};
					rpd.colorAttachmentCount = atts.size();
					rpd.colorAttachments = atts.data();
					if (passData.depthTarget)
						rpd.depthStencilAttachment = &rpdsa;
					rpd.label = label.c_str();
					renderEnc = cmdEnc.BeginRenderPass(&rpd);
				}
				statistics.passes++;
			}

			void scissors(Vec2i origin, Vec2i size) { renderEnc.SetScissorRect(origin[0], origin[1], size[0], size[1]); }

			void draw(DrawConfig config)
			{
				CAGE_ASSERT(cmdEnc && renderEnc);
				CAGE_ASSERT(config.shader);
				CAGE_ASSERT(config.model);
				CAGE_ASSERT(config.model->bindings());
				if (!config.bindings)
					config.bindings = getEmptyBindings(device);

				wgpu::RenderPipeline pip = newGraphicsPipeline(device, convertPipelineConfig(passData, config));
				if (!pip)
					return; // pipeline not ready

				if (passData.pipeline.Get() != pip.Get())
				{
					renderEnc.SetPipeline(pip);
					renderEnc.SetBindGroup(0, passData.bindings.group);
					passData.pipeline = pip;
					statistics.pipelineSwitches++;
				}

				renderEnc.SetBindGroup(1, config.materialOverride ? config.materialOverride->group : config.model->bindings().group);
				CAGE_ASSERT(config.bindings.dynamicBuffersCount <= config.dynamicOffsets.size());
				renderEnc.SetBindGroup(2, config.bindings.group, config.bindings.dynamicBuffersCount, config.dynamicOffsets.data());

				renderEnc.SetVertexBuffer(0, config.model->geometryBuffer->nativeBuffer());
				if (config.model->indicesCount)
					renderEnc.SetIndexBuffer(config.model->geometryBuffer->nativeBuffer(), wgpu::IndexFormat::Uint32, config.model->indicesOffset);

				if (config.model->indicesCount)
					renderEnc.DrawIndexed(config.model->indicesCount, config.instances);
				else
					renderEnc.Draw(config.model->verticesCount, config.instances);

				statistics.drawCalls++;
				statistics.primitives += config.model->primitivesCount * config.instances;
			}

			void submit()
			{
				const ProfilingScope profiling("encoder submit");
				CAGE_ASSERT(cmdEnc && renderEnc);
				renderEnc.End();
				renderEnc = nullptr;
				wgpu::CommandBuffer b = cmdEnc.Finish();
				cmdEnc = nullptr;
				device->insertCommandBuffer(std::move(b), statistics);
				statistics = {};
			}
		};
	}

	namespace detail
	{
		RenderEncoderNamedScope::RenderEncoderNamedScope(GraphicsEncoder *queue, StringPointer name) : queue(queue)
		{
			GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)queue;
			impl->renderEnc.PushDebugGroup((const char *)name);
		}

		RenderEncoderNamedScope::~RenderEncoderNamedScope()
		{
			GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)queue;
			impl->renderEnc.PopDebugGroup();
		}
	}

	void GraphicsEncoder::nextPass(const RenderPassConfig &config)
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		impl->nextPass(config);
	}

	detail::RenderEncoderNamedScope GraphicsEncoder::namedScope(StringPointer name)
	{
		return detail::RenderEncoderNamedScope(this, name);
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

	const wgpu::CommandEncoder &GraphicsEncoder::nativeCommandEncoder()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		return impl->cmdEnc;
	}

	const wgpu::RenderPassEncoder &GraphicsEncoder::nativeRenderEncoder()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		return impl->renderEnc;
	}

	Holder<GraphicsEncoder> newGraphicsEncoder(GraphicsDevice *device, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsEncoder, GraphicsEncoderImpl>(device, label);
	}
}
