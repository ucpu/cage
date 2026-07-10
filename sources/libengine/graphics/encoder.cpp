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
			gpu::CommandEncoder cmdEnc;
			gpu::RenderPassEncoder renderEnc;
			PassData passData;
			GraphicsFrameStatistics statistics;
			ankerl::svector<StringPointer, 4> debugScopes;

			GraphicsEncoderImpl(GraphicsDevice *device, const AssetLabel &label_) : device(device)
			{
				this->label = label_;
				CAGE_ASSERT(device);
			}

			void nextPass(const RenderPassConfig &config)
			{
				const ProfilingScope profiling("encoder next pass");
				if (renderEnc)
				{
					for (const auto it : debugScopes)
					{
						(void)it;
						renderEnc.popDebugGroup();
					}

					renderEnc.end();
					renderEnc = {};
					passData = {};
				}
				if (!cmdEnc)
				{
					gpu::CommandEncoderDescriptor desc = {};
					desc.label = label;
					cmdEnc = device->nativeDevice()->createCommandEncoder(desc);
				}
				{
					passData = PassData{ config };
					if (!passData.bindings)
						passData.bindings = privat::getEmptyBindings(device);

					ankerl::svector<gpu::RenderPassDescriptor::ColorAttachment, 1> atts;
					atts.reserve(passData.colorTargets.size());
					for (const auto &it : passData.colorTargets)
					{
						gpu::RenderPassDescriptor::ColorAttachment rpca = {};
						rpca.clearValue = it.clearValue;
						rpca.loadOp = it.clear ? gpu::LoadOp::Clear : gpu::LoadOp::Load;
						rpca.storeOp = gpu::StoreOp::Store;
						CAGE_ASSERT(it.texture->nativeView());
						rpca.view = it.texture->nativeView();
						atts.push_back(std::move(rpca));
					}

					gpu::RenderPassDescriptor::DepthStencilAttachment rpdsa = {};
					if (passData.depthTarget)
					{
						CAGE_ASSERT(passData.depthTarget->texture->nativeView());
						rpdsa.view = passData.depthTarget->texture->nativeView();
						rpdsa.depthLoadOp = passData.depthTarget->clear ? gpu::LoadOp::Clear : gpu::LoadOp::Load;
						rpdsa.depthStoreOp = gpu::StoreOp::Store;
						rpdsa.depthClearValue = 1;
					}

					gpu::RenderPassDescriptor rpd = {};
					rpd.colorAttachments = atts;
					if (passData.depthTarget)
						rpd.depthStencilAttachment = std::move(rpdsa);
					rpd.label = label;
					renderEnc = cmdEnc.beginRenderPass(rpd);

					for (const auto it : debugScopes)
						renderEnc.pushDebugGroup((const char *)it);
				}
				statistics.passes++;
			}

			void scissors(Vec2i origin, Vec2i size) { renderEnc.setScissorRect(origin[0], origin[1], size[0], size[1]); }

			void draw(DrawConfig config)
			{
				CAGE_ASSERT(cmdEnc && renderEnc);
				CAGE_ASSERT(config.shader);
				CAGE_ASSERT(config.model);
				if (!config.material)
					config.material = newGraphicsBindings(device, nullptr, config.model).first;
				if (!config.bindings)
					config.bindings = privat::getEmptyBindings(device);

				gpu::RenderPipeline pip = newGraphicsPipeline(device, convertPipelineConfig(passData, config));
				if (!pip)
					return; // pipeline not ready

				if (passData.pipeline.get() != pip.get())
				{
					renderEnc.setPipeline(pip);
					renderEnc.setBindGroup(0, passData.bindings.group);
					passData.pipeline = pip;
					statistics.pipelineSwitches++;
				}

				renderEnc.setBindGroup(1, config.material.group);
				CAGE_ASSERT(config.bindings.dynamicBuffersCount <= config.dynamicOffsets.size());
				renderEnc.setBindGroup(2, config.bindings.group, PointerRange<const uint32>(config.dynamicOffsets).subRange(0, config.bindings.dynamicBuffersCount));

				renderEnc.setVertexBuffer(0, config.model->geometryBuffer->nativeBuffer());
				if (config.model->indicesCount)
					renderEnc.setIndexBuffer(config.model->geometryBuffer->nativeBuffer(), gpu::IndexFormat::Uint32, config.model->indicesOffset);

				if (config.model->indicesCount)
					renderEnc.drawIndexed(config.model->indicesCount, config.instances);
				else
					renderEnc.draw(config.model->verticesCount, config.instances);

				statistics.drawCalls++;
				statistics.primitives += config.model->primitivesCount * config.instances;
			}

			void submit()
			{
				const ProfilingScope profiling("encoder submit");
				CAGE_ASSERT(cmdEnc && renderEnc);
				renderEnc.end();
				renderEnc = {};
				gpu::CommandBuffer b = cmdEnc.finish();
				cmdEnc = {};
				device->insertCommandBuffer(std::move(b), statistics);
				statistics = {};
			}

			void pushScope(StringPointer name)
			{
				CAGE_ASSERT(renderEnc);
				debugScopes.push_back(name);
				renderEnc.pushDebugGroup((const char *)name);
			}

			void popScope()
			{
				CAGE_ASSERT(renderEnc);
				debugScopes.pop_back();
				renderEnc.popDebugGroup();
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

	gpu::CommandEncoder &GraphicsEncoder::nativeCommandEncoder()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		return impl->cmdEnc;
	}

	gpu::RenderPassEncoder &GraphicsEncoder::nativeRenderEncoder()
	{
		GraphicsEncoderImpl *impl = (GraphicsEncoderImpl *)this;
		return impl->renderEnc;
	}

	Holder<GraphicsEncoder> newGraphicsEncoder(GraphicsDevice *device, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsEncoder, GraphicsEncoderImpl>(device, label);
	}
}
