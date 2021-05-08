#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/serialization.h>

#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h> // GL_TEXTURE_2D_ARRAY

#include "private.h"

#include <optick.h>

namespace cage
{
	void SkinData::bind(RenderQueue *queue) const
	{
		queue->bind(uubRange, 0);
		queue->bind(texture.share(), 0);
	}

	void RenderableBase::setClip(const HierarchyItem *item)
	{
		clipPos = item->clipPos * item->impl->pointsScale;
		clipSize = item->clipSize * item->impl->pointsScale;
	}

	bool RenderableBase::prepare()
	{
		if (clipSize[0] >= 1 && clipSize[1] >= 1)
		{
			const ivec2 pos = ivec2(vec2(clipPos[0], impl->outputResolution[1] - clipPos[1] - clipSize[1]));
			impl->activeQueue->scissors(pos, ivec2(clipSize));
			return true;
		}
		return false;
	}

	RenderableElement::RenderableElement(WidgetItem *item, GuiElementTypeEnum element, uint32 mode, vec2 pos, vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(element < GuiElementTypeEnum::TotalElements);
		CAGE_ASSERT(mode < 4);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		setClip(item->hierarchy);
		data.element = (uint32)element;
		data.mode = mode;
		data.outer = item->hierarchy->impl->pointsToNdc(pos, size);
		const vec4 &border = item->skin->layouts[(uint32)element].border;
		offset(pos, size, -border);
		data.inner = item->hierarchy->impl->pointsToNdc(pos, size);
		skin = item->skin;
	}

	RenderableElement::~RenderableElement()
	{
		if (!prepare())
			return;
		RenderQueue *q = impl->activeQueue;
		skin->bind(q);
		q->bind(impl->graphicsData.elementShader.share());
		q->uniform(0, data.outer);
		q->uniform(1, data.inner);
		q->uniform(2, data.element);
		q->uniform(3, data.mode);
		q->bind(impl->graphicsData.elementModel.share());
		q->draw();
	}

	RenderableText::RenderableText(TextItem *item, vec2 position, vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(item->color.valid());
		setClip(item->hierarchy);
		data.transform = item->transform;
		data.format = item->format;
		data.font = item->font.share();
		item->glyphs = std::move(item->glyphs).makeShareable();
		data.glyphs = item->glyphs.share();
		data.color = item->color;
		data.cursor = item->cursor;
		data.format.size *= item->hierarchy->impl->pointsScale;
		const ivec2 &orr = item->hierarchy->impl->outputResolution;
		position *= item->hierarchy->impl->pointsScale;
		data.transform = transpose(mat4(
			2.0 / orr[0], 0, 0, 2.0 * position[0] / orr[0] - 1.0,
			0, 2.0 / orr[1], 0, 1.0 - 2.0 * position[1] / orr[1],
			0, 0, 1, 0,
			0, 0, 0, 1
		));
		data.format.wrapWidth = size[0] * item->hierarchy->impl->pointsScale;
	}

	RenderableText::~RenderableText()
	{
		if (!prepare())
			return;
		RenderQueue *q = impl->activeQueue;
		q->bind(impl->graphicsData.fontShader.share());
		q->uniform(0, data.transform);
		q->uniform(4, data.color);
		data.font->bind(q, impl->graphicsData.fontModel.share(), impl->graphicsData.fontShader.share());
		data.font->render(q, data.glyphs, data.format, data.cursor);
	}

	RenderableImage::RenderableImage(ImageItem *item, vec2 position, vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		setClip(item->hierarchy);
		data.texture = item->texture.share();
		data.ndcPos = item->hierarchy->impl->pointsToNdc(position, size);
		data.uvClip = vec4(item->image.textureUvOffset, item->image.textureUvOffset + item->image.textureUvSize);
		// todo format mode
		data.aniTexFrames = detail::evalSamplesForTextureAnimation(+item->texture, applicationTime(), item->image.animationStart, item->format.animationSpeed, item->format.animationOffset);
	}

	RenderableImage::~RenderableImage()
	{
		if (!prepare())
			return;
		RenderQueue *q = impl->activeQueue;
		q->bind(data.texture.share(), 0);
		q->bind(data.texture->getTarget() == GL_TEXTURE_2D_ARRAY ? impl->graphicsData.imageAnimatedShader.share() : impl->graphicsData.imageStaticShader.share());
		q->uniform(0, data.ndcPos);
		q->uniform(1, data.uvClip);
		if (data.texture->getTarget() == GL_TEXTURE_2D_ARRAY)
			q->uniform(2, data.aniTexFrames);
		q->bind(impl->graphicsData.imageModel.share());
		q->draw();
	}

	void GuiImpl::GraphicsData::load(AssetManager *assetMgr)
	{
		elementShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/element.glsl"));
		fontShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
		imageAnimatedShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?A"));
		imageStaticShader = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/image.glsl?a"));
		colorPickerShader[0] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?F"));
		colorPickerShader[1] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?H"));
		colorPickerShader[2] = assetMgr->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?S"));
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
			GuiImpl *impl = nullptr;

			GraphicsDataCleaner(GuiImpl *impl) : impl(impl)
			{}

			~GraphicsDataCleaner()
			{
				impl->graphicsData = GuiImpl::GraphicsData();
				for (auto &it : impl->skins)
					it.texture.clear();
			}
		};
	}

	void GuiImpl::emit()
	{
		OPTICK_EVENT("render gui");

		if (outputResolution[0] <= 0 || outputResolution[1] <= 0)
			return;

		if (!assetMgr->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")))
			return;

		GraphicsDataCleaner graphicsDataCleaner(this);

		for (auto &s : skins)
		{
			s.texture = assetMgr->get<AssetSchemeIndexTexture, Texture>(s.textureName);
			if (!s.texture)
				return;
		}

		graphicsData.load(assetMgr);

		CAGE_ASSERT(activeQueue);
		RenderQueue *q = activeQueue;
		auto namedScope = q->scopedNamedPass("gui");

		// write skins uv coordinates
		for (auto &s : skins)
		{
			GuiSkinElementLayout::TextureUv textureUvs[(uint32)GuiElementTypeEnum::TotalElements];
			for (uint32 i = 0; i < (uint32)GuiElementTypeEnum::TotalElements; i++)
				copyTextureUv(s.layouts[i].textureUv, textureUvs[i]);
			s.uubRange = q->universalUniformArray<GuiSkinElementLayout::TextureUv>(textureUvs);
		}

		// render all
		q->scissors(true);
		q->blending(true);
		q->blendFuncAlphaTransparency();
		q->depthTest(false);
		q->viewport(ivec2(), outputResolution);

		root->childrenEmit();

		q->blending(false);
		q->scissors(false);
	}
}
