#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include <cage-client/opengl.h>
#include "../private.h"

namespace cage
{
	void renderableElementStruct::render(guiImpl *impl)
	{
		guiImpl::graphicDataStruct &context = impl->graphicData;
		skinBuffer->bind(0);
		skinTexture->bind();
		context.guiShader->bind();
		context.guiShader->uniform(0, data.outer);
		context.guiShader->uniform(1, data.inner);
		context.guiShader->uniform(2, data.element);
		context.guiShader->uniform(3, data.mode);
		context.guiMesh->bind();
		context.guiMesh->dispatch();
	}

	void renderableTextStruct::render(guiImpl *impl)
	{
		guiImpl::graphicDataStruct &context = impl->graphicData;
		data.font->bind(context.fontMesh, context.fontShader, impl->outputResolution[0], impl->outputResolution[1]);
		data.font->render(data.glyphs, data.count, data.format, data.pos, data.color, data.cursor);
	}

	void renderableImageStruct::render(guiImpl *impl)
	{
		guiImpl::graphicDataStruct &context = impl->graphicData;
		data.texture->bind();
		shaderClass *shr = data.texture->getTarget() == GL_TEXTURE_2D_ARRAY ? context.imageAnimatedShader : context.imageStaticShader;
		shr->bind();
		shr->uniform(0, data.ndcPos);
		shr->uniform(1, data.uvClip);
		if (data.texture->getTarget() == GL_TEXTURE_2D_ARRAY)
			shr->uniform(2, data.aniTexFrames);
		context.imageMesh->bind();
		context.imageMesh->dispatch();
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

		void copyTextureUv(const skinElementLayoutStruct::textureUvOiStruct &source, skinElementLayoutStruct::textureUvOiStruct &target)
		{
			copyTextureUv(source.inner, target.inner);
			copyTextureUv(source.outer, target.outer);
		}

		void copyTextureUv(const skinElementLayoutStruct::textureUvStruct &source, skinElementLayoutStruct::textureUvStruct &target)
		{
			for (int i = 0; i < 4; i++)
				copyTextureUv(source.data[i], target.data[i]);
		}
	}

	void guiImpl::graphicsDispatch()
	{
		CAGE_ASSERT_RUNTIME(openglContext);

		if ((emitIndexDispatch + 1) % 3 != emitIndexControl)
			emitIndexDispatch = (emitIndexDispatch + 1) % 3;

		// check skins textures
		for (auto &s : skins)
		{
			if (!s.texture)
				return;
		}

		// write skins uv coordinates
		for (auto &s : skins)
		{
			skinElementLayoutStruct::textureUvStruct textureUvs[(uint32)elementTypeEnum::TotalElements];
			for (uint32 i = 0; i < (uint32)elementTypeEnum::TotalElements; i++)
				copyTextureUv(s.layouts[i].textureUv, textureUvs[i]);
			s.elementsGpuBuffer->bind();
			s.elementsGpuBuffer->writeRange(textureUvs, 0, sizeof(textureUvs));
		}

		// render all
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);
		glViewport(0, 0, outputResolution.x, outputResolution.y);

		renderableBaseStruct *r = emitData[emitIndexDispatch].first;
		while (r)
		{
			r->render(this);
			r = r->next;
		}

		glDisable(GL_BLEND);
	}
}
