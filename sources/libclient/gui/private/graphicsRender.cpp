#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/swapBufferController.h>

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
		guiImpl::graphicsDataStruct &context = impl->graphicsData;
		skinBuffer->bind(0);
		skinTexture->bind();
		context.elementShader->bind();
		context.elementShader->uniform(0, data.outer);
		context.elementShader->uniform(1, data.inner);
		context.elementShader->uniform(2, data.element);
		context.elementShader->uniform(3, data.mode);
		context.elementMesh->bind();
		context.elementMesh->dispatch();
	}

	void renderableTextStruct::render(guiImpl *impl)
	{
		guiImpl::graphicsDataStruct &context = impl->graphicsData;
		data.font->bind(context.fontMesh, context.fontShader);
		context.fontShader->uniform(0, data.transform);
		context.fontShader->uniform(4, data.color);
		data.font->render(data.glyphs, data.count, data.format, data.cursor);
	}

	void renderableImageStruct::render(guiImpl *impl)
	{
		guiImpl::graphicsDataStruct &context = impl->graphicsData;
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
		CAGE_CHECK_GL_ERROR_DEBUG();

		if (auto lock = emitController->read())
		{
			if (outputResolution.x <= 0 || outputResolution.y <= 0)
				return;

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
			CAGE_CHECK_GL_ERROR_DEBUG();
			glEnable(GL_SCISSOR_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_DEPTH_TEST);
			glActiveTexture(GL_TEXTURE0);
			glViewport(0, 0, outputResolution.x, outputResolution.y);
			renderableBaseStruct *r = emitData[lock.index()].first;
			while (r)
			{
				if (r->clipSize[0] >= 1 && r->clipSize[1] >= 1)
				{
					glScissor(numeric_cast<sint32>(r->clipPos[0]), numeric_cast<sint32>(real(outputResolution.y) - r->clipPos[1] - r->clipSize[1]), numeric_cast<uint32>(r->clipSize[0]), numeric_cast<uint32>(r->clipSize[1]));
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
			CAGE_LOG_DEBUG(severityEnum::Warning, "gui", "gui is skipping render because control was late");
		}
	}
}
