#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/serialization.h>

#include <cage-engine/opengl.h>
#include <cage-engine/graphics.h>
#include "../private.h"

#include <optick.h>

namespace cage
{
	void RenderableElement::render(GuiImpl *impl)
	{
		GuiImpl::GraphicsData &context = impl->graphicsData;
		skin->elementsGpuBuffer->bind(0);
		skin->texture->bind();
		context.elementShader->bind();
		context.elementShader->uniform(0, data.outer);
		context.elementShader->uniform(1, data.inner);
		context.elementShader->uniform(2, data.element);
		context.elementShader->uniform(3, data.mode);
		context.elementModel->bind();
		context.elementModel->dispatch();
	}

	void RenderableText::render(GuiImpl *impl)
	{
		GuiImpl::GraphicsData &context = impl->graphicsData;
		context.fontShader->bind();
		context.fontShader->uniform(0, data.transform);
		context.fontShader->uniform(4, data.color);
		data.font->render(context.fontModel.share(), context.fontShader.share() , { data.glyphs, data.glyphs + data.count }, data.format, data.cursor);
	}

	void RenderableImage::render(GuiImpl *impl)
	{
		GuiImpl::GraphicsData &context = impl->graphicsData;
		data.texture->bind();
		Holder<ShaderProgram> shr = data.texture->getTarget() == GL_TEXTURE_2D_ARRAY ? context.imageAnimatedShader.share() : context.imageStaticShader.share();
		shr->bind();
		shr->uniform(0, data.ndcPos);
		shr->uniform(1, data.uvClip);
		if (data.texture->getTarget() == GL_TEXTURE_2D_ARRAY)
			shr->uniform(2, data.aniTexFrames);
		context.imageModel->bind();
		context.imageModel->dispatch();
	}

	void GuiImpl::GraphicsData::load(AssetManager *assetMgr)
	{
		debugShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/debug.glsl"));
		elementShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/element.glsl"));
		fontShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
		imageAnimatedShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?A"));
		imageStaticShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?a"));
		colorPickerShader[0] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?F"));
		colorPickerShader[1] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?H"));
		colorPickerShader[2] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?S"));
		debugModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/guiWire.obj"));
		elementModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/guiElement.obj"));
		fontModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
		imageModel = fontModel.share();
	}

	namespace
	{
		void copyTextureUv(const vec4 &source, vec4 &target)
		{
			target[0] = source[0];
			target[2] = source[2];
			target[1] = 1 - source[3]; // these are swapped intentionally
			target[3] = 1 - source[1];
		}

		void copyTextureUv(const GuiSkinElementLayout::TextureUvOi &source, GuiSkinElementLayout::TextureUvOi &target)
		{
			copyTextureUv(source.inner, target.inner);
			copyTextureUv(source.outer, target.outer);
		}

		void copyTextureUv(const GuiSkinElementLayout::TextureUv &source, GuiSkinElementLayout::TextureUv &target)
		{
			for (int i = 0; i < 4; i++)
				copyTextureUv(source.data[i], target.data[i]);
		}

		struct GraphicsDataCleaner : Immovable
		{
			GuiImpl &data;

			GraphicsDataCleaner(GuiImpl &data) : data(data)
			{}

			~GraphicsDataCleaner()
			{
				data.graphicsData = GuiImpl::GraphicsData();
				for (auto &it : data.skins)
					it.texture.clear();
			}
		};
	}

	void GuiImpl::graphicsDispatch()
	{
		GraphicsDebugScope graphicsDebugScope("render gui");
		OPTICK_EVENT("render gui");
		CAGE_CHECK_GL_ERROR_DEBUG();

		GraphicsDataCleaner graphicsDataCleaner(*this);

		if (auto lock = emitController->read())
		{
			if (outputResolution[0] <= 0 || outputResolution[1] <= 0)
				return;

			if (!assetMgr->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")))
				return;

			for (auto &s : skins)
			{
				s.texture = assetMgr->get<AssetSchemeIndexTexture, Texture>(s.textureName);
				if (!s.texture)
					return;
			}

			graphicsData.load(assetMgr);

			// write skins uv coordinates
			for (auto &s : skins)
			{
				GuiSkinElementLayout::TextureUv textureUvs[(uint32)GuiElementTypeEnum::TotalElements];
				for (uint32 i = 0; i < (uint32)GuiElementTypeEnum::TotalElements; i++)
					copyTextureUv(s.layouts[i].textureUv, textureUvs[i]);
				s.elementsGpuBuffer->bind();
				s.elementsGpuBuffer->writeRange(bufferCast<const char, GuiSkinElementLayout::TextureUv>(textureUvs), 0);
			}

			// render all
			CAGE_CHECK_GL_ERROR_DEBUG();
			glEnable(GL_SCISSOR_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_DEPTH_TEST);
			glActiveTexture(GL_TEXTURE0);
			glViewport(0, 0, outputResolution[0], outputResolution[1]);
			RenderableBase *r = emitData[lock.index()].first;
			while (r)
			{
				if (r->clipSize[0] >= 1 && r->clipSize[1] >= 1)
				{
					glScissor(numeric_cast<sint32>(r->clipPos[0]), numeric_cast<sint32>(real(outputResolution[1]) - r->clipPos[1] - r->clipSize[1]), numeric_cast<uint32>(r->clipSize[0]), numeric_cast<uint32>(r->clipSize[1]));
					r->render(this);
				}
				r = r->next;
			}
			glDisable(GL_BLEND);
			glDisable(GL_SCISSOR_TEST);
			CAGE_CHECK_GL_ERROR_DEBUG();
		}
		else
		{
			//CAGE_LOG_DEBUG(SeverityEnum::Warning, "gui", "gui is skipping render because control was late");
		}
	}
}
