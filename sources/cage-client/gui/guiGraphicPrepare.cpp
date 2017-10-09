#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/assets.h>
#include <cage-client/graphic.h>
#include <cage-client/opengl.h>
#include "private.h"

namespace cage
{
	namespace
	{
		struct dispatchCacheStruct
		{
			dispatchCacheStruct *next;
			dispatchCacheStruct() : next(nullptr) {}
			virtual void dispatch(guiImpl *context) {};
		};

		struct dispatchElementStruct : public dispatchCacheStruct
		{
			struct elementStruct
			{
				vec4 outer;
				vec4 inner;
				uint32 element;
				uint32 mode;
			} data;

			virtual void dispatch(guiImpl *context)
			{
				context->guiShader->bind();
				context->guiShader->uniform(0, data.outer);
				context->guiShader->uniform(1, data.inner);
				context->guiShader->uniform(2, data.element);
				context->guiShader->uniform(3, data.mode);
				context->guiMesh->bind();
				context->guiMesh->dispatch();
			}
		};

		struct dispatchTextStruct : public dispatchCacheStruct
		{
			struct textStruct
			{
				uint32 *glyphs;
				fontClass *font;
				fontClass::formatStruct format;
				vec3 color;
				uint32 cursor;
				uint32 count;
				sint16 posX, posY;
			} data;

			virtual void dispatch(guiImpl *context)
			{
				data.font->bind(context->fontMesh, context->fontShader, numeric_cast<uint32>(context->windowSize[0]), numeric_cast<uint32>(context->windowSize[1]));
				data.font->render(data.glyphs, data.count, data.format, data.posX, data.posY, data.color, data.cursor);
			}
		};

		struct dispatchImageStruct : public dispatchCacheStruct
		{
			struct imageStruct
			{
				vec4 ndcPos;
				vec4 uvClip;
				vec4 aniTexFrames;
				textureClass *texture;
			} data;

			virtual void dispatch(guiImpl *context)
			{
				data.texture->bind();
				shaderClass *shr = data.texture->getTarget() == GL_TEXTURE_2D_ARRAY ? context->imageAnimatedShader : context->imageStaticShader;
				shr->bind();
				shr->uniform(0, data.ndcPos);
				shr->uniform(1, data.uvClip);
				if (data.texture->getTarget() == GL_TEXTURE_2D_ARRAY)
					shr->uniform(2, data.aniTexFrames);
				context->imageMesh->bind();
				context->imageMesh->dispatch();
			}
		};

		void copyTextureUv(const vec4 &source, vec4 &target)
		{
			target[0] = source[0];
			target[2] = source[2];
			target[1] = 1 - source[3]; // these are swapped intentionally
			target[3] = 1 - source[1];
		}

		void copyTextureUv(const elementLayoutStruct::textureUvStruct::textureUvOiStruct &source, elementLayoutStruct::textureUvStruct::textureUvOiStruct &target)
		{
			copyTextureUv(source.inner, target.inner);
			copyTextureUv(source.outer, target.outer);
		}

		void copyTextureUv(const elementLayoutStruct::textureUvStruct &source, elementLayoutStruct::textureUvStruct &target)
		{
			copyTextureUv(source.defal, target.defal);
			copyTextureUv(source.disab, target.disab);
			copyTextureUv(source.focus, target.focus);
			copyTextureUv(source.hover, target.hover);
		}
	}

	void guiImpl::performGraphicPrepare(uint64 time)
	{
		prepareTime = time;
		prepareCacheFirst = prepareCacheLast = nullptr;
		skinTexture = nullptr;
		guiShader = fontShader = imageAnimatedShader = imageStaticShader = nullptr;
		guiMesh = fontMesh = imageMesh = nullptr;

		if (!cacheRoot)
			return;
		if (!assetManager->ready(hashString("cage/cage.pack")))
			return;
		if (!assetManager->ready(skinName))
			return;

		for (uint32 i = 0; i < (uint32)elementTypeEnum::TotalElements; i++)
			copyTextureUv(elements[i].textureUv, textureUvs[i]);

		skinTexture = assetManager->get<assetSchemeIndexTexture, textureClass>(skinName);
		guiShader = assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui.glsl"));
		fontShader = assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/font.glsl"));
		imageAnimatedShader = assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/image.glsl?A"));
		imageStaticShader = assetManager->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/image.glsl?a"));
		guiMesh = assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/gui.obj"));
		fontMesh = assetManager->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
		imageMesh = fontMesh;

		memoryPrepare.flush();
		prepareCacheFirst = prepareCacheLast = memoryPrepare.createObject<dispatchCacheStruct>();
		cacheRoot->graphicPrepare();
	}

	void guiImpl::performGraphicDispatch()
	{
		CAGE_ASSERT_RUNTIME(!!elementsGpuBuffer, "gui rendering not initialized");

		if (!skinTexture)
			return;

		glActiveTexture(GL_TEXTURE1);
		skinTexture->bind();
		glActiveTexture(GL_TEXTURE0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		elementsGpuBuffer->bind();
		elementsGpuBuffer->writeRange(textureUvs, 0, sizeof(textureUvs));
		elementsGpuBuffer->bind(0);

		dispatchCacheStruct *e = prepareCacheFirst;
		while (e)
		{
			e->dispatch(this);
			e = e->next;
		}

		glDisable(GL_BLEND);
	}

	void guiImpl::prepareElement(const vec4 &outer, const vec4 &inner, elementTypeEnum element, controlCacheStruct::controlModeEnum mode)
	{
		CAGE_ASSERT_RUNTIME(outer.valid(), outer);
		CAGE_ASSERT_RUNTIME(inner.valid(), inner);
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements, element);
		CAGE_ASSERT_RUNTIME(mode == controlCacheStruct::cmNormal || mode == controlCacheStruct::cmHover || mode == controlCacheStruct::cmFocus || mode == controlCacheStruct::cmDisabled, mode);
		dispatchElementStruct *e = memoryPrepare.createObject<dispatchElementStruct>();
		e->data.outer = outer;
		e->data.inner = inner;
		e->data.element = (uint32)element;
		e->data.mode = mode;
		prepareCacheLast->next = e;
		prepareCacheLast = e;
	}

	void guiImpl::prepareText(const itemCacheStruct *text, sint16 pixelsPosX, sint16 pixelsPosY, uint16 pixelsWidth, uint16 pixelsHeight, uint32 cursor)
	{
		CAGE_ASSERT_RUNTIME(text->type == itemCacheStruct::itText);
		if (!text->font || !text->translatedText || (text->translatedLength == 0 && cursor != 0))
			return;
		dispatchTextStruct *t = memoryPrepare.createObject<dispatchTextStruct>();
		t->data.color = vec3((real*)text->color);
		t->data.font = text->font;
		t->data.format.align = text->align;
		t->data.format.lineSpacing = text->lineSpacing;
		t->data.format.wrapWidth = pixelsWidth;
		t->data.count = text->translatedLength;
		t->data.cursor = cursor;
		t->data.glyphs = (uint32*)memoryPrepare.allocate(text->translatedLength * sizeof(uint32));
		detail::memcpy(t->data.glyphs, text->translatedText, text->translatedLength * sizeof(uint32));
		t->data.posX = pixelsPosX;
		t->data.posY = pixelsPosY;
		prepareCacheLast->next = t;
		prepareCacheLast = t;
	}

	void guiImpl::prepareImage(const itemCacheStruct *image, vec4 ndcPosSize)
	{
		CAGE_ASSERT_RUNTIME(image->type == itemCacheStruct::itImage);
		if (!image->texture)
			return;
		dispatchImageStruct *i = memoryPrepare.createObject<dispatchImageStruct>();
		i->data.ndcPos = ndcPosSize;
		i->data.texture = image->texture;
		copyTextureUv(vec4((real*)image->uvClip), i->data.uvClip);
		i->data.aniTexFrames = detail::evalSamplesForTextureAnimation(image->texture, prepareTime, image->animationStart, image->animationSpeed, image->animationOffset);
		prepareCacheLast->next = i;
		prepareCacheLast = i;
	}
}