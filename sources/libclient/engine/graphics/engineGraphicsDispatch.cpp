#include <vector>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/engine.h>
#include <cage-client/opengl.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/window.h>

#include "../engine.h"
#include "graphics.h"

namespace cage
{
	namespace
	{
		configSint32 visualizeBuffer("cage-client.engine.visualizeBuffer", 0);

		struct shadowmapBufferStruct
		{
		private:
			uint32 width;
			uint32 height;
		public:
			holder<textureClass> texture;
			void resize(uint32 w, uint32 h)
			{
				if (w == width && h == height)
					return;
				width = w;
				height = h;
				texture->bind();
				if (texture->getTarget() == GL_TEXTURE_CUBE_MAP)
					texture->imageCube(w, h, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_HALF_FLOAT, nullptr);
				else
					texture->image2d(w, h, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_HALF_FLOAT, nullptr);
				texture->filters(GL_LINEAR, GL_LINEAR, 0);
				texture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				//glTexParameteri(texture->getTarget(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				//glTexParameteri(texture->getTarget(), GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			shadowmapBufferStruct(uint32 target) : width(0), height(0)
			{
				CAGE_ASSERT_RUNTIME(target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D);
				texture = newTexture(window(), target);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		};

		enum class visualizableTextureModeEnum
		{
			Color,
			Depth2d,
			DepthCube,
			Velocity,
		};

		struct visualizableTextureStruct
		{
			textureClass *tex;
			visualizableTextureModeEnum visualizableTextureMode;
			visualizableTextureStruct(textureClass *tex, visualizableTextureModeEnum vtm) : tex(tex), visualizableTextureMode(vtm) {};
		};

		struct graphicsDispatchImpl : public graphicsDispatchStruct
		{
		public:
			uint32 drawCalls;
			uint32 drawPrimitives;

		private:
			uint32 gBufferWidth;
			uint32 gBufferHeight;

			holder<frameBufferClass> gBufferTarget;
			holder<frameBufferClass> renderTarget;
			holder<textureClass> albedoTexture;
			holder<textureClass> specialTexture;
			holder<textureClass> normalTexture;
			holder<textureClass> colorTexture;
			holder<textureClass> intermediateTexture;
			holder<textureClass> velocityTexture;
			holder<textureClass> depthTexture;

			holder<uniformBufferClass> viewportDataBuffer;
			holder<uniformBufferClass> meshDataBuffer;
			holder<uniformBufferClass> armatureDataBuffer;
			holder<uniformBufferClass> lightsDataBuffer;

			std::vector<shadowmapBufferStruct> shadowmaps2d, shadowmapsCube;
			std::vector<visualizableTextureStruct> visualizableTextures;

			bool lastTwoSided;
			bool lastDepthTest;
			bool lastDepthWrite;

			static void applyShaderRoutines(shaderConfigStruct *c, shaderClass *s)
			{
				for (uint32 i = 0; i < MaxRoutines; i++)
				{
					if (c->shaderRoutineStage[i])
						s->subroutine(c->shaderRoutineStage[i], c->shaderRoutineName[i], c->shaderRoutineValue[i]);
				}
				s->applySubroutines();
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void setTwoSided(bool twoSided)
			{
				if (twoSided != lastTwoSided)
				{
					if (twoSided)
						glDisable(GL_CULL_FACE);
					else
						glEnable(GL_CULL_FACE);
					lastTwoSided = twoSided;
				}
			}

			void setDepthTest(bool depthTest, bool depthWrite)
			{
				if (depthTest != lastDepthTest)
				{
					if (depthTest)
						glDepthFunc(GL_LEQUAL);
					else
						glDepthFunc(GL_ALWAYS);
					lastDepthTest = depthTest;
				}
				if (depthWrite != lastDepthWrite)
				{
					if (depthWrite)
						glDepthMask(GL_TRUE);
					else
						glDepthMask(GL_FALSE);
					lastDepthWrite = depthWrite;
				}
			}

			void gBufferResize(uint32 w, uint32 h)
			{
				if (w == gBufferWidth && h == gBufferHeight)
					return;
				gBufferWidth = w;
				gBufferHeight = h;
				albedoTexture->bind(); albedoTexture->image2d(w, h, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				specialTexture->bind(); specialTexture->image2d(w, h, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, nullptr);
				normalTexture->bind(); normalTexture->image2d(w, h, GL_RGB16F, GL_RGB, GL_HALF_FLOAT, nullptr);
				colorTexture->bind(); colorTexture->image2d(w, h, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				intermediateTexture->bind(); intermediateTexture->image2d(w, h, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				velocityTexture->bind(); velocityTexture->image2d(w, h, GL_RG16F, GL_RG, GL_HALF_FLOAT, nullptr);
				depthTexture->bind(); depthTexture->image2d(w, h, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderObject(objectsStruct *obj, shaderClass *shr)
			{
				applyShaderRoutines(&obj->shaderConfig, shr);
				meshDataBuffer->bind();
				meshDataBuffer->writeRange(obj->shaderMeshes, 0, sizeof(objectsStruct::shaderMeshStruct) * obj->count);
				if (obj->shaderArmatures)
				{
					armatureDataBuffer->bind();
					armatureDataBuffer->writeRange(obj->shaderArmatures, 0, sizeof(mat3x4) * obj->count * obj->mesh->getSkeletonBones());
					shr->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, obj->mesh->getSkeletonBones());
				}
				obj->mesh->bind();
				setTwoSided((obj->mesh->getFlags() & meshFlags::TwoSided) == meshFlags::TwoSided);
				setDepthTest((obj->mesh->getFlags() & meshFlags::DepthTest) == meshFlags::DepthTest, (obj->mesh->getFlags() & meshFlags::DepthWrite) == meshFlags::DepthWrite);
				{ // bind textures
					uint32 tius[MaxTexturesCountPerMaterial];
					textureClass *texs[MaxTexturesCountPerMaterial];
					for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					{
						if (obj->textures[i])
						{
							tius[i] = (obj->textures[i]->getTarget() == GL_TEXTURE_2D_ARRAY ? CAGE_SHADER_TEXTURE_ALBEDO_ARRAY : CAGE_SHADER_TEXTURE_ALBEDO) + i;
							texs[i] = obj->textures[i];
						}
						else
						{
							tius[i] = 0;
							texs[i] = nullptr;
						}
					}
					textureClass::multiBind(MaxTexturesCountPerMaterial, tius, texs);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
				obj->mesh->dispatch(obj->count);
				drawCalls++;
				drawPrimitives += obj->count * obj->mesh->getPrimitivesCount();
			}

			void renderOpaque(renderPassStruct *pass)
			{
				CAGE_ASSERT_RUNTIME(pass->targetShadowmap == 0);

				gBufferTarget->bind();
				gBufferTarget->drawAttachments(31);
				gBufferTarget->checkStatus();
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (pass->clearFlags)
					glClear(pass->clearFlags);

				shaderClass *shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				shr->bind();
				for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
					renderObject(o, shr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void bindGBufferTextures()
			{
				const uint32 tius[5] = { CAGE_SHADER_TEXTURE_ALBEDO, CAGE_SHADER_TEXTURE_SPECIAL, CAGE_SHADER_TEXTURE_NORMAL, CAGE_SHADER_TEXTURE_VELOCITY, CAGE_SHADER_TEXTURE_DEPTH };
				textureClass *texs[5] = { albedoTexture.get(), specialTexture.get(), normalTexture.get(), velocityTexture.get(), depthTexture.get() };
				textureClass::multiBind(5, tius, texs);
			}

			void renderLighting(renderPassStruct *pass)
			{
				CAGE_ASSERT_RUNTIME(pass->targetShadowmap == 0);

				renderTarget->bind();
				renderTarget->depthTexture(nullptr);
				renderTarget->colorTexture(0, colorTexture.get());
				renderTarget->drawAttachments(1);
				renderTarget->checkStatus();
				bindGBufferTextures();
				CAGE_CHECK_GL_ERROR_DEBUG();

				setTwoSided(false);
				setDepthTest(false, false);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				CAGE_CHECK_GL_ERROR_DEBUG();

				shaderClass *shr = shaderLighting;
				shr->bind();
				for (lightsStruct *l = pass->firstLighting; l; l = l->next)
				{
					applyShaderRoutines(&l->shaderConfig, shr);
					meshClass *mesh = nullptr;
					switch (l->lightType)
					{
					case lightTypeEnum::Directional:
						mesh = meshSquare;
						glCullFace(GL_BACK);
						break;
					case lightTypeEnum::Spot:
						mesh = meshCone;
						glCullFace(GL_FRONT);
						break;
					case lightTypeEnum::Point:
						mesh = meshSphere;
						glCullFace(GL_FRONT);
						break;
					default:
						CAGE_THROW_CRITICAL(exception, "invalid light type");
					}
					if (l->shadowmap != 0)
					{
						glActiveTexture(GL_TEXTURE0 + (l->shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE));
						shadowmapBufferStruct &s = l->shadowmap > 0 ? shadowmaps2d[l->shadowmap - 1] : shadowmapsCube[-l->shadowmap - 1];
						s.texture->bind();
					}
					lightsDataBuffer->bind();
					lightsDataBuffer->writeRange(l->shaderLights, 0, sizeof(lightsStruct::shaderLightStruct) * l->count);
					mesh->bind();
					mesh->dispatch(l->count);
					drawCalls++;
					drawPrimitives += l->count * mesh->getPrimitivesCount();
				}

				glCullFace(GL_BACK);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTranslucent(renderPassStruct *pass)
			{
				CAGE_ASSERT_RUNTIME(pass->targetShadowmap == 0);

				gBufferTarget->bind();
				gBufferTarget->drawAttachments(1 << CAGE_SHADER_ATTRIB_OUT_COLOR);
				gBufferTarget->checkStatus();
				CAGE_CHECK_GL_ERROR_DEBUG();

				setDepthTest(true, true);
				glEnable(GL_BLEND);
				CAGE_CHECK_GL_ERROR_DEBUG();

				shaderClass *shr = shaderTranslucent;
				shr->bind();
				for (translucentStruct *t = pass->firstTranslucent; t; t = t->next)
				{
					{ // render ambient object
						glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // assume premultiplied alpha
						shr->subroutine(GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, CAGE_SHADER_ROUTINEPROC_LIGHTAMBIENT);
						renderObject(&t->object, shr);
					}

					{ // render lights on the object
					  /*
					  glBlendFunc(GL_ONE, GL_ONE); // assume premultiplied alpha
					  for (lightsStruct *l = t->firstLight; l; l = l->next)
					  {
					  l->setShaderRoutines(shr);
					  shr->applySubroutines();
					  // todo
					  }
					  */
					}
				}

				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderEffectPrepare(textureClass *&texSource, textureClass *&texTarget)
			{
				renderTarget->colorTexture(0, texTarget);
				renderTarget->checkStatus();
				texSource->bind();
				std::swap(texSource, texTarget);
			}

			void renderPass(renderPassStruct *pass)
			{
				viewportDataBuffer->bind();
				viewportDataBuffer->writeRange(&pass->shaderViewport, 0, sizeof(renderPassStruct::shaderViewportStruct));
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_SCISSOR_TEST);
				glDisable(GL_BLEND);
				glViewport(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				glScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				CAGE_CHECK_GL_ERROR_DEBUG();

				// reset the state
				lastTwoSided = true;
				setTwoSided(false);
				lastDepthTest = false; lastDepthWrite = false;
				setDepthTest(true, true);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (pass->targetShadowmap == 0)
				{ // deferred renderer
					renderOpaque(pass);
					if (pass->firstLighting)
						renderLighting(pass);
					if (pass->firstTranslucent)
						renderTranslucent(pass);
				}
				else
				{ // shadowmap
					renderTarget->bind();
					renderTarget->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1].texture.get() : shadowmapsCube[-pass->targetShadowmap - 1].texture.get());
					renderTarget->colorTexture(0, nullptr);
					renderTarget->drawAttachments(0);
					renderTarget->checkStatus();
					glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
					glClear(GL_DEPTH_BUFFER_BIT);
					CAGE_CHECK_GL_ERROR_DEBUG();

					shaderClass *shr = shaderDepth;
					shr->bind();
					for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
						renderObject(o, shr);

					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				setTwoSided(false);
				setDepthTest(false, false);
				glDisable(GL_BLEND);
				glActiveTexture(GL_TEXTURE0);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (pass->targetShadowmap)
					return;

				renderTarget->bind();
				renderTarget->depthTexture(nullptr);
				renderTarget->drawAttachments(1);

				textureClass *texSource = colorTexture.get();
				textureClass *texTarget = intermediateTexture.get();
				if (pass->effects != cameraEffectsFlags::None)
				{ // all screen-space post-processing
					bindGBufferTextures();
					if ((pass->effects & cameraEffectsFlags::MotionBlur) == cameraEffectsFlags::MotionBlur)
					{
						renderEffectPrepare(texSource, texTarget);
						shaderMotionBlur->bind();
						meshSquare->bind();
						meshSquare->dispatch();
						CAGE_CHECK_GL_ERROR_DEBUG();
					}
				}

				if (texSource != pass->targetTexture)
				{ // blit to final output texture
					renderTarget->colorTexture(0, pass->targetTexture);
					renderTarget->checkStatus();
					texSource->bind();
					shaderBlitColor->bind();
					uint32 w = gBufferWidth, h = gBufferHeight;
					shaderBlitColor->uniform(0, vec4(pass->vpX, pass->vpY, pass->vpW, pass->vpH) / vec4(w, h, w, h));
					meshSquare->bind();
					meshSquare->dispatch();
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
			}

		public:
			graphicsDispatchImpl(const engineCreateConfig &config) : drawCalls(0), drawPrimitives(0), gBufferWidth(0), gBufferHeight(0), lastTwoSided(false), lastDepthTest(false)
			{
				detail::memset(static_cast<graphicsDispatchStruct*>(this), 0, sizeof(graphicsDispatchStruct));
			}

			void initialize()
			{
				shadowmaps2d.reserve(4);
				shadowmapsCube.reserve(8);

				gBufferWidth = gBufferHeight = 0;

#define GCHL_GENERATE(NAME) NAME = newTexture(window()); NAME->filters(GL_LINEAR, GL_LINEAR, 0);
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, albedoTexture, specialTexture, normalTexture, colorTexture, intermediateTexture, velocityTexture, depthTexture));
#undef GCHL_GENERATE
				CAGE_CHECK_GL_ERROR_DEBUG();

				gBufferTarget = newFrameBuffer(window());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, velocityTexture.get());
				gBufferTarget->depthTexture(depthTexture.get());
				renderTarget = newFrameBuffer(window());
				CAGE_CHECK_GL_ERROR_DEBUG();

				viewportDataBuffer = newUniformBuffer(window());
				viewportDataBuffer->writeWhole(nullptr, sizeof(renderPassStruct::shaderViewportStruct), GL_DYNAMIC_DRAW);
				meshDataBuffer = newUniformBuffer(window());
				meshDataBuffer->writeWhole(nullptr, sizeof(objectsStruct::shaderMeshStruct) * CAGE_SHADER_MAX_INSTANCES, GL_DYNAMIC_DRAW);
				armatureDataBuffer = newUniformBuffer(window());
				armatureDataBuffer->writeWhole(nullptr, sizeof(mat3x4) * CAGE_SHADER_MAX_BONES, GL_DYNAMIC_DRAW);
				lightsDataBuffer = newUniformBuffer(window());
				lightsDataBuffer->writeWhole(nullptr, sizeof(lightsStruct::shaderLightStruct) * CAGE_SHADER_MAX_INSTANCES, GL_DYNAMIC_DRAW);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void finalize()
			{
				gBufferWidth = gBufferHeight = 0;

				gBufferTarget.clear();
				renderTarget.clear();
				albedoTexture.clear();
				specialTexture.clear();
				normalTexture.clear();
				colorTexture.clear();
				intermediateTexture.clear();
				velocityTexture.clear();
				depthTexture.clear();
				shadowmaps2d.clear();
				shadowmapsCube.clear();

				viewportDataBuffer.clear();
				meshDataBuffer.clear();
				armatureDataBuffer.clear();
				lightsDataBuffer.clear();
			}

			void tick()
			{
				drawCalls = 0;
				drawPrimitives = 0;

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (windowWidth == 0 || windowHeight == 0)
					return;

				if (!shaderBlitColor)
				{
					glClear(GL_COLOR_BUFFER_BIT);
					CAGE_CHECK_GL_ERROR_DEBUG();
					return;
				}

				visualizableTextures.clear();
				visualizableTextures.emplace_back(colorTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(albedoTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(specialTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(normalTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(velocityTexture.get(), visualizableTextureModeEnum::Velocity);
				visualizableTextures.emplace_back(depthTexture.get(), visualizableTextureModeEnum::Depth2d);

				{ // prepare all render targets
					uint32 maxW = windowWidth, maxH = windowHeight;
					for (renderPassStruct *renderPass = firstRenderPass; renderPass; renderPass = renderPass->next)
					{
						if (renderPass->targetTexture)
						{ // render to texture
							visualizableTextures.emplace_back(renderPass->targetTexture, visualizableTextureModeEnum::Color);
							uint32 w, h;
							renderPass->targetTexture->getResolution(w, h);
							maxW = max(maxW, w);
							maxH = max(maxH, h);
							continue;
						}
						if (renderPass->targetShadowmap == 0)
						{ // render to screen
							renderPass->targetTexture = colorTexture.get();
							continue;
						}
						// render to shadowmap
						if (renderPass->targetShadowmap > 0)
						{ // 2d shadowmap
							uint32 idx = renderPass->targetShadowmap - 1;
							while (shadowmaps2d.size() <= idx)
								shadowmaps2d.push_back(shadowmapBufferStruct(GL_TEXTURE_2D));
							shadowmapBufferStruct &s = shadowmaps2d[idx];
							s.resize(renderPass->shadowmapResolution, renderPass->shadowmapResolution);
							visualizableTextures.emplace_back(s.texture.get(), visualizableTextureModeEnum::Depth2d);
						}
						if (renderPass->targetShadowmap < 0)
						{ // cube shadowmap
							uint32 idx = -renderPass->targetShadowmap - 1;
							while (shadowmapsCube.size() <= idx)
								shadowmapsCube.push_back(shadowmapBufferStruct(GL_TEXTURE_CUBE_MAP));
							shadowmapBufferStruct &s = shadowmapsCube[idx];
							s.resize(renderPass->shadowmapResolution, renderPass->shadowmapResolution);
							visualizableTextures.emplace_back(s.texture.get(), visualizableTextureModeEnum::DepthCube);
						}
					}
					gBufferResize(maxW, maxH);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				viewportDataBuffer->bind(CAGE_SHADER_UNIBLOCK_VIEWPORT);
				meshDataBuffer->bind(CAGE_SHADER_UNIBLOCK_MESHES);
				armatureDataBuffer->bind(CAGE_SHADER_UNIBLOCK_ARMATURES);
				lightsDataBuffer->bind(CAGE_SHADER_UNIBLOCK_LIGHTS);
				CAGE_CHECK_GL_ERROR_DEBUG();

				{ // render all passes
					for (renderPassStruct *pass = firstRenderPass; pass; pass = pass->next)
						renderPass(pass);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				{ // blit to the window
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glViewport(0, 0, windowWidth, windowHeight);
					glScissor(0, 0, windowWidth, windowHeight);
					setTwoSided(false);
					setDepthTest(false, false);
					sint32 visualizeIndex = visualizeBuffer % numeric_cast<sint32>(visualizableTextures.size());
					if (visualizeIndex < 0)
						visualizeIndex += numeric_cast<sint32>(visualizableTextures.size());
					visualizableTextureStruct &v = visualizableTextures[visualizeIndex];
					v.tex->bind();
					switch (v.visualizableTextureMode)
					{
					case visualizableTextureModeEnum::Color:
						shaderBlitColor->bind();
						shaderBlitColor->uniform(0, vec4(0, 0, 1, 1));
						break;
					case visualizableTextureModeEnum::Depth2d:
					case visualizableTextureModeEnum::DepthCube:
						shaderBlitDepth->bind();
						shaderBlitDepth->uniform(0, vec4(0, 0, 1, 1));
						break;
					case visualizableTextureModeEnum::Velocity:
						shaderBlitVelocity->bind();
						shaderBlitVelocity->uniform(0, vec4(0, 0, 1, 1));
						break;
					}
					meshSquare->bind();
					meshSquare->dispatch();
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				{ // check gl errors (even in release, but do not halt the game)
					try
					{
						checkGlError();
					}
					catch (const graphicsException &)
					{
					}
				}
			}

			void swap()
			{
				CAGE_CHECK_GL_ERROR_DEBUG();
				window()->swapBuffers();
				glFinish(); // this is where the engine should be waiting for the gpu
			}
		};
	}

	graphicsDispatchStruct *graphicsDispatch;

	void graphicsDispatchCreate(const engineCreateConfig &config)
	{
		graphicsDispatch = detail::systemArena().createObject<graphicsDispatchImpl>(config);
	}

	void graphicsDispatchDestroy()
	{
		detail::systemArena().destroy<graphicsDispatchImpl>(graphicsDispatch);
		graphicsDispatch = nullptr;
	}

	void graphicsDispatchInitialize()
	{
		((graphicsDispatchImpl*)graphicsDispatch)->initialize();
	}

	void graphicsDispatchFinalize()
	{
		((graphicsDispatchImpl*)graphicsDispatch)->finalize();
	}

	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives)
	{
		graphicsDispatchImpl *impl = (graphicsDispatchImpl*)graphicsDispatch;
		impl->tick();
		drawCalls = impl->drawCalls;
		drawPrimitives = impl->drawPrimitives;
	}

	void graphicsDispatchSwap()
	{
		((graphicsDispatchImpl*)graphicsDispatch)->swap();
	}
}
