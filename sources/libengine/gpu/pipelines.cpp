#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		RenderPipelineImpl::RenderPipelineImpl(const DeviceImpl &device, const RenderPipelineDescriptor &desc) : layout(desc.layout)
		{
			ankerl::svector<vk::PipelineShaderStageCreateInfo, 2> stages;
			{
				vk::PipelineShaderStageCreateInfo ci;
				ci.module = *desc.vertex.module->shader;
				ci.stage = vk::ShaderStageFlagBits::eVertex;
				ci.pName = "main";
				stages.push_back(std::move(ci));
			}
			if (desc.fragment)
			{
				vk::PipelineShaderStageCreateInfo ci;
				ci.module = *desc.fragment->module->shader;
				ci.stage = vk::ShaderStageFlagBits::eFragment;
				ci.pName = "main";
				stages.push_back(std::move(ci));
			}

			uint32 binding = 0;
			ankerl::svector<vk::VertexInputBindingDescription, 1> vertexBindings;
			ankerl::svector<vk::VertexInputAttributeDescription, 5> vertexAttributes;
			for (const auto &b : desc.vertex.buffers)
			{
				vk::VertexInputBindingDescription bd;
				bd.binding = binding;
				bd.inputRate = convertVertexStepMode(b.stepMode);
				bd.stride = b.arrayStride;
				for (const auto &a : b.attributes)
				{
					vk::VertexInputAttributeDescription ad;
					ad.binding = binding;
					ad.format = convertVertexFormat(a.format);
					ad.location = a.shaderLocation;
					ad.offset = a.offset;
					vertexAttributes.push_back(ad);
				}
				vertexBindings.push_back(bd);
				binding++;
			}
			vk::PipelineVertexInputStateCreateInfo vertexInput;
			vertexInput.vertexBindingDescriptionCount = vertexBindings.size();
			vertexInput.pVertexBindingDescriptions = vertexBindings.data();
			vertexInput.vertexAttributeDescriptionCount = vertexAttributes.size();
			vertexInput.pVertexAttributeDescriptions = vertexAttributes.data();

			vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
			inputAssembly.topology = convertPrimitiveTopology(desc.primitive.topology);

			vk::PipelineViewportStateCreateInfo viewport;
			viewport.viewportCount = 1;
			viewport.scissorCount = 1;

			vk::PipelineRasterizationStateCreateInfo raster;
			raster.cullMode = convertCullMode(desc.primitive.cullMode);
			raster.frontFace = vk::FrontFace::eCounterClockwise;
			raster.polygonMode = vk::PolygonMode::eFill;
			raster.lineWidth = 1;

			vk::PipelineMultisampleStateCreateInfo msaa;
			msaa.rasterizationSamples = vk::SampleCountFlagBits::e1;

			vk::PipelineDepthStencilStateCreateInfo depth;
			if (desc.depthStencil)
			{
				depth.depthTestEnable = true;
				depth.depthWriteEnable = desc.depthStencil->depthWriteEnabled;
				depth.depthCompareOp = convertCompareFunction(desc.depthStencil->depthCompare);
			}

			ankerl::svector<vk::PipelineColorBlendAttachmentState, 4> attachments;
			if (desc.fragment)
			{
				for (const auto &it : desc.fragment->targets)
				{
					vk::PipelineColorBlendAttachmentState as;
					as.blendEnable = !!it.blend;
					if (it.blend)
					{
						as.colorBlendOp = convertBlendOperation(it.blend->color.operation);
						as.alphaBlendOp = convertBlendOperation(it.blend->alpha.operation);
						as.srcColorBlendFactor = convertBlendFactor(it.blend->color.srcFactor);
						as.srcAlphaBlendFactor = convertBlendFactor(it.blend->alpha.srcFactor);
						as.dstColorBlendFactor = convertBlendFactor(it.blend->color.dstFactor);
						as.dstAlphaBlendFactor = convertBlendFactor(it.blend->alpha.dstFactor);
					}
					as.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
					attachments.push_back(as);
				}
			}
			vk::PipelineColorBlendStateCreateInfo blend;
			blend.attachmentCount = attachments.size();
			blend.pAttachments = attachments.data();

			std::array<vk::DynamicState, 2> dynamicEntries = {
				vk::DynamicState::eViewport,
				vk::DynamicState::eScissor,
			};
			vk::PipelineDynamicStateCreateInfo dynamic;
			dynamic.dynamicStateCount = dynamicEntries.size();
			dynamic.pDynamicStates = dynamicEntries.data();

			ankerl::svector<vk::Format, 4> colorFormats;
			if (desc.fragment)
			{
				for (const auto &t : desc.fragment->targets)
					colorFormats.push_back(convertTextureFormat(t.format));
			}
			vk::PipelineRenderingCreateInfo rendering;
			rendering.colorAttachmentCount = colorFormats.size();
			rendering.pColorAttachmentFormats = colorFormats.data();
			rendering.depthAttachmentFormat = desc.depthStencil ? convertTextureFormat(desc.depthStencil->format) : vk::Format::eUndefined;
			rendering.stencilAttachmentFormat = vk::Format::eUndefined;

			vk::GraphicsPipelineCreateInfo ci;
			ci.stageCount = stages.size();
			ci.pStages = stages.data();
			ci.pVertexInputState = &vertexInput;
			ci.pInputAssemblyState = &inputAssembly;
			ci.pViewportState = &viewport;
			ci.pRasterizationState = &raster;
			ci.pMultisampleState = &msaa;
			ci.pDepthStencilState = &depth;
			ci.pColorBlendState = &blend;
			ci.pDynamicState = &dynamic;
			ci.pNext = &rendering;
			ci.layout = *layout->layout;
			auto r = device.device.createGraphicsPipelineUnique({}, ci);
			check("createGraphicsPipelineUnique", r.result);
			CAGE_ASSERT(r.has_value());
			pipeline = std::move(r.value);
		}

		RenderPipelineImpl::~RenderPipelineImpl() {}

		ShaderModuleImpl::ShaderModuleImpl(const DeviceImpl &device, const ShaderModuleDescriptor &desc)
		{
			vk::ShaderModuleCreateInfo ci;
			ci.codeSize = desc.spirvCode.size() * sizeof(uint32);
			ci.pCode = desc.spirvCode.data();
			shader = device.device.createShaderModuleUnique(ci);
		}

		ShaderModuleImpl::~ShaderModuleImpl() {}

		PipelineLayoutImpl::PipelineLayoutImpl(const DeviceImpl &device, const PipelineLayoutDescriptor &desc)
		{
			ankerl::svector<vk::DescriptorSetLayout, 5> ls;
			ls.reserve(desc.bindGroupLayouts.size());
			for (const auto &it : desc.bindGroupLayouts)
				ls.push_back(*it->layout);
			vk::PipelineLayoutCreateInfo ci;
			ci.setLayoutCount = ls.size();
			ci.pSetLayouts = ls.data();
			layout = device.device.createPipelineLayoutUnique(ci);
		}

		PipelineLayoutImpl::~PipelineLayoutImpl() {}
	}
}
