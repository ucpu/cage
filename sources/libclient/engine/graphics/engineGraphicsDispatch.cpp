#include <vector>
#include <map>
#include <set>

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
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			shadowmapBufferStruct(uint32 target) : width(0), height(0)
			{
				CAGE_ASSERT_RUNTIME(target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D);
				texture = newTexture(target);
				texture->filters(GL_LINEAR, GL_LINEAR, 16);
				texture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
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

		struct ssaoShaderStruct
		{
			mat4 viewProj;
			mat4 viewProjInv;
			vec4 ambientLight;
			vec4 params; // strength, bias, power, radius
			uint32 iparams[4];
		};

		struct finalScreenShaderStruct
		{
			cameraTonemapStruct tonemap; // 7 reals
			real tonemapEnabled;

			real eyeAdaptationKey;
			real eyeAdaptationStrength;
			real _dummy1;
			real _dummy2;

			real gamma;
			real _dummy3;
			real _dummy4;
			real _dummy5;
		};

		struct cameraSpecificDataStruct
		{
			holder<textureClass> luminanceCollectionTexture; // w*h
			holder<textureClass> luminanceAccumulationTexture; // 1*1

			cameraSpecificDataStruct() : width(0), height(0)
			{}

			void update(uint32 w, uint32 h)
			{
				if (!luminanceCollectionTexture)
				{
					luminanceCollectionTexture = newTexture();
					luminanceCollectionTexture->filters(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0);
					luminanceCollectionTexture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					luminanceAccumulationTexture = newTexture();
					luminanceAccumulationTexture->filters(GL_NEAREST, GL_NEAREST, 0);
					luminanceAccumulationTexture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					luminanceAccumulationTexture->image2d(1, 1, GL_R16F);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				if (w != width || h != height)
				{
					width = w;
					height = h;
					luminanceCollectionTexture->bind();
					luminanceCollectionTexture->image2d(max(w / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u), max(h / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u), GL_R16F);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
			}

		private:
			uint32 width;
			uint32 height;
		};

		struct graphicsDispatchHolders
		{
			holder<frameBufferClass> gBufferTarget;
			holder<frameBufferClass> renderTarget;
			holder<textureClass> albedoTexture;
			holder<textureClass> specialTexture;
			holder<textureClass> normalTexture;
			holder<textureClass> colorTexture;
			holder<textureClass> intermediateTexture;
			holder<textureClass> velocityTexture;
			holder<textureClass> ambientOcclusionTexture1;
			holder<textureClass> ambientOcclusionTexture2;
			holder<textureClass> bloomTexture1;
			holder<textureClass> bloomTexture2;
			holder<textureClass> depthTexture;

			holder<uniformBufferClass> viewportDataBuffer;
			holder<uniformBufferClass> meshDataBuffer;
			holder<uniformBufferClass> armatureDataBuffer;
			holder<uniformBufferClass> lightsDataBuffer;
			holder<uniformBufferClass> ssaoDataBuffer;
			holder<uniformBufferClass> finalScreenDataBuffer;

			std::vector<shadowmapBufferStruct> shadowmaps2d, shadowmapsCube;
			std::vector<visualizableTextureStruct> visualizableTextures;
			std::map<uintPtr, cameraSpecificDataStruct> cameras;
		};

		struct graphicsDispatchImpl : public graphicsDispatchStruct, private graphicsDispatchHolders
		{
		public:
			uint32 drawCalls;
			uint32 drawPrimitives;

		private:
			uint32 gBufferWidth;
			uint32 gBufferHeight;

			bool lastTwoSided;
			bool lastDepthTest;
			bool lastDepthWrite;
			
			textureClass *texSource;
			textureClass *texTarget;

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

			static void viewportAndScissor(sint32 x, sint32 y, uint32 w, uint32 h)
			{
				glViewport(x, y, w, h);
				glScissor(x, y, w, h);
			}

			static void resetAllTextures()
			{
				for (uint32 i = 0; i < 16; i++)
				{
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_1D, 0);
					glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
					glBindTexture(GL_TEXTURE_3D, 0);
				}
				glActiveTexture(GL_TEXTURE0);
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

			void bindGBufferTextures()
			{
				const uint32 tius[] = { CAGE_SHADER_TEXTURE_ALBEDO, CAGE_SHADER_TEXTURE_SPECIAL, CAGE_SHADER_TEXTURE_NORMAL, CAGE_SHADER_TEXTURE_COLOR, CAGE_SHADER_TEXTURE_DEPTH };
				static const uint32 cnt = sizeof(tius) / sizeof(tius[0]);
				textureClass *texs[cnt] = { albedoTexture.get(), specialTexture.get(), normalTexture.get(), colorTexture.get(), depthTexture.get() };
				textureClass::multiBind(cnt, tius, texs);
			}

			void bindShadowmap(sint32 shadowmap)
			{
				if (shadowmap != 0)
				{
					glActiveTexture(GL_TEXTURE0 + (shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE));
					shadowmapBufferStruct &s = shadowmap > 0 ? shadowmaps2d[shadowmap - 1] : shadowmapsCube[-shadowmap - 1];
					s.texture->bind();
				}
			}

			void renderEffect()
			{
				renderTarget->colorTexture(0, texTarget);
#ifdef CAGE_DEBUG
				renderTarget->checkStatus();
#endif // CAGE_DEBUG
				texSource->bind();
				renderDispatch(meshSquare, 1);
				CAGE_CHECK_GL_ERROR_DEBUG();
				std::swap(texSource, texTarget);
			}

			void renderDispatch(objectsStruct *obj)
			{
				renderDispatch(obj->mesh, obj->count);
			}

			void renderDispatch(meshClass *mesh, uint32 count)
			{
				mesh->dispatch(count);
				drawCalls++;
				drawPrimitives += count * mesh->getPrimitivesCount();
			}

			void renderObject(objectsStruct *obj, shaderClass *shr)
			{
				renderObject(obj, shr, obj->mesh->getFlags());
			}

			void renderObject(objectsStruct *obj, shaderClass *shr, meshFlags flags)
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
				setTwoSided((flags & meshFlags::TwoSided) == meshFlags::TwoSided);
				setDepthTest((flags & meshFlags::DepthTest) == meshFlags::DepthTest, (flags & meshFlags::DepthWrite) == meshFlags::DepthWrite);
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
				renderDispatch(obj);
			}

			void renderOpaque(renderPassStruct *pass)
			{
				shaderClass *shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				shr->bind();
				for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
					renderObject(o, shr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderLighting(renderPassStruct *pass)
			{
				shaderClass *shr = shaderLighting;
				shr->bind();
				for (lightsStruct *l = pass->firstLight; l; l = l->next)
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
					bindShadowmap(l->shadowmap);
					lightsDataBuffer->bind();
					lightsDataBuffer->writeRange(l->shaderLights, 0, sizeof(lightsStruct::shaderLightStruct) * l->count);
					mesh->bind();
					renderDispatch(mesh, l->count);
				}
				glCullFace(GL_BACK);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTranslucent(renderPassStruct *pass)
			{
				shaderClass *shr = shaderTranslucent;
				shr->bind();
				for (translucentStruct *t = pass->firstTranslucent; t; t = t->next)
				{
					{ // render ambient object
						glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // assume premultiplied alpha
						shr->subroutine(GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, CAGE_SHADER_ROUTINEPROC_LIGHTAMBIENT);
						renderObject(&t->object, shr);
					}

					if (t->firstLight)
					{ // render lights on the object
						lightsDataBuffer->bind();
						glBlendFunc(GL_ONE, GL_ONE); // assume premultiplied alpha
						for (lightsStruct *l = t->firstLight; l; l = l->next)
						{
							applyShaderRoutines(&l->shaderConfig, shr);
							bindShadowmap(l->shadowmap);
							lightsDataBuffer->writeRange(l->shaderLights, 0, sizeof(lightsStruct::shaderLightStruct) * l->count);
							renderDispatch(t->object.mesh, l->count);
						}
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTexts(renderPassStruct *pass)
			{
				for (textsStruct *t = pass->firstText; t; t = t->next)
				{
					t->font->bind(meshSquare, shaderFont);
					for (textsStruct::renderStruct *r = t->firtsRender; r; r = r->next)
					{
						shaderFont->uniform(0, r->transform);
						shaderFont->uniform(4, r->color);
						t->font->render(r->glyphs, r->count, r->format);
						drawCalls += (r->count + CAGE_SHADER_MAX_CHARACTERS - 1) / CAGE_SHADER_MAX_CHARACTERS;
						drawPrimitives += r->count * 2;
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderCameraPass(renderPassStruct *pass)
			{
				// camera specific data
				cameraSpecificDataStruct &cs = cameras[pass->entityId];
				cs.update(pass->vpW, pass->vpH);

				// opaque
				gBufferTarget->bind();
				gBufferTarget->activeAttachments(31);
				gBufferTarget->checkStatus();
				CAGE_CHECK_GL_ERROR_DEBUG();
				if (pass->clearFlags)
					glClear(pass->clearFlags);
				renderOpaque(pass);
				setTwoSided(false);
				setDepthTest(false, false);
				CAGE_CHECK_GL_ERROR_DEBUG();

				// lighting
				renderTarget->bind();
				renderTarget->clear();
				renderTarget->colorTexture(0, colorTexture.get());
				renderTarget->activeAttachments(1);
				renderTarget->checkStatus();
				bindGBufferTextures();
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				CAGE_CHECK_GL_ERROR_DEBUG();
				renderLighting(pass);
				setDepthTest(false, false);
				glDisable(GL_BLEND);
				glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
				CAGE_CHECK_GL_ERROR_DEBUG();

				// opaque screen-space effects
				renderTarget->bind();
				renderTarget->depthTexture(nullptr);
				renderTarget->activeAttachments(1);
				bindGBufferTextures();
				meshSquare->bind();
				texSource = colorTexture.get();
				texTarget = intermediateTexture.get();
				CAGE_CHECK_GL_ERROR_DEBUG();

				// ssao
				if ((pass->effects & cameraEffectsFlags::AmbientOcclusion) == cameraEffectsFlags::AmbientOcclusion)
				{
					viewportAndScissor(pass->vpX / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpY / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpW / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpH / CAGE_SHADER_SSAO_DOWNSCALE);
					{
						ssaoShaderStruct s;
						s.viewProj = pass->viewProj;
						s.viewProjInv = pass->viewProj.inverse();
						s.params = vec4(pass->ssao.strength, pass->ssao.bias, pass->ssao.power, pass->ssao.worldRadius);
						s.ambientLight = vec4(pass->shaderViewport.ambientLight);
						s.iparams[0] = pass->ssao.samplesCount;
						ssaoDataBuffer->bind();
						ssaoDataBuffer->writeRange(&s, 0, sizeof(ssaoShaderStruct));
					}
					{ // generate
						renderTarget->colorTexture(0, ambientOcclusionTexture1.get());
						renderTarget->checkStatus();
						shaderSsaoGenerate->bind();
						renderDispatch(meshSquare, 1);
					}
					{ // blur
						shaderSsaoBlur->bind();
						auto ssaoBlur = [&](holder<textureClass> &tex1, holder<textureClass> &tex2, const vec2 &direction)
						{
							tex1->bind();
							renderTarget->colorTexture(0, tex2.get());
							renderTarget->checkStatus();
							shaderSsaoBlur->uniform(0, direction);
							renderDispatch(meshSquare, 1);
						};
						ssaoBlur(ambientOcclusionTexture1, ambientOcclusionTexture2, vec2(pass->ssao.blurRadius, 0.0));
						ssaoBlur(ambientOcclusionTexture2, ambientOcclusionTexture1, vec2(0.0, pass->ssao.blurRadius));
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					{ // apply
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_EFFECTS);
						ambientOcclusionTexture1->bind();
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
						shaderSsaoApply->bind();
						renderEffect();
					}
				}

				// motion blur
				if ((pass->effects & cameraEffectsFlags::MotionBlur) == cameraEffectsFlags::MotionBlur)
				{
					glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_EFFECTS);
					velocityTexture->bind();
					glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
					shaderMotionBlur->bind();
					renderEffect();
				}

				// eye adaptation
				if ((pass->effects & cameraEffectsFlags::EyeAdaptation) == cameraEffectsFlags::EyeAdaptation)
				{
					// bind the luminance texture for use
					{
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_EFFECTS);
						cs.luminanceAccumulationTexture->bind();
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
					}
					// luminance collection
					{
						viewportAndScissor(pass->vpX / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpY / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpW / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpH / CAGE_SHADER_LUMINANCE_DOWNSCALE);
						renderTarget->colorTexture(0, cs.luminanceCollectionTexture.get());
						renderTarget->checkStatus();
						texSource->bind();
						shaderLuminanceCollection->bind();
						renderDispatch(meshSquare, 1);
					}
					// downscale
					{
						cs.luminanceCollectionTexture->bind();
						cs.luminanceCollectionTexture->generateMipmaps();
					}
					// luminance copy
					{
						viewportAndScissor(0, 0, 1, 1);
						renderTarget->colorTexture(0, cs.luminanceAccumulationTexture.get());
						renderTarget->checkStatus();
						shaderLuminanceCopy->bind();
						shaderLuminanceCopy->uniform(0, vec2(pass->eyeAdaptation.darkerSpeed, pass->eyeAdaptation.lighterSpeed));
						renderDispatch(meshSquare, 1);
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				}

				// transparencies
				if (!!pass->firstTranslucent || !!pass->firstText)
				{
					if (texSource != colorTexture.get())
					{
						shaderBlit->bind();
						renderEffect();
					}
					CAGE_ASSERT_RUNTIME(texSource == colorTexture.get());
					CAGE_ASSERT_RUNTIME(texTarget == intermediateTexture.get());

					renderTarget->colorTexture(0, colorTexture.get());
					renderTarget->depthTexture(depthTexture.get());
					renderTarget->activeAttachments(1);
					renderTarget->checkStatus();
					CAGE_CHECK_GL_ERROR_DEBUG();

					setDepthTest(true, true);
					glEnable(GL_BLEND);
					CAGE_CHECK_GL_ERROR_DEBUG();

					renderTranslucent(pass);

					setDepthTest(true, false);
					setTwoSided(true);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					CAGE_CHECK_GL_ERROR_DEBUG();

					renderTexts(pass);

					setDepthTest(false, false);
					setTwoSided(false);
					glDisable(GL_BLEND);
					glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
					CAGE_CHECK_GL_ERROR_DEBUG();

					renderTarget->depthTexture(nullptr);
					renderTarget->activeAttachments(1);
					meshSquare->bind();
					bindGBufferTextures();
				}

				// bloom
				if ((pass->effects & cameraEffectsFlags::Bloom) == cameraEffectsFlags::Bloom)
				{
					viewportAndScissor(pass->vpX / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpY / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpW / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpH / CAGE_SHADER_BLOOM_DOWNSCALE);
					{ // generate
						renderTarget->colorTexture(0, bloomTexture1.get());
						renderTarget->checkStatus();
						shaderBloomGenerate->bind();
						shaderBloomGenerate->uniform(0, vec4(pass->bloom.threshold, 0, 0, 0));
						renderDispatch(meshSquare, 1);
					}
					{ // blur
						shaderBloomBlur->bind();
						auto bloomBlur = [&](holder<textureClass> &tex1, holder<textureClass> &tex2, const vec2 &direction)
						{
							tex1->bind();
							renderTarget->colorTexture(0, tex2.get());
							renderTarget->checkStatus();
							shaderBloomBlur->uniform(0, direction);
							renderDispatch(meshSquare, 1);
						};
						for (uint32 i = 0; i < pass->bloom.blurPasses; i++)
						{
							bloomBlur(bloomTexture1, bloomTexture2, vec2(pass->bloom.blurRadius, 0.0));
							bloomBlur(bloomTexture2, bloomTexture1, vec2(0.0, pass->bloom.blurRadius));
						}
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					{ // apply
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_EFFECTS);
						bloomTexture1->bind();
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
						CAGE_CHECK_GL_ERROR_DEBUG();
						shaderBloomApply->bind();
						renderEffect();
					}
				}

				// final screen effects
				if ((pass->effects & (cameraEffectsFlags::EyeAdaptation | cameraEffectsFlags::ToneMapping | cameraEffectsFlags::GammaCorrection)) != cameraEffectsFlags::None)
				{
					finalScreenShaderStruct f;
					if ((pass->effects & cameraEffectsFlags::EyeAdaptation) == cameraEffectsFlags::EyeAdaptation)
					{
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_EFFECTS);
						cs.luminanceAccumulationTexture->bind();
						glActiveTexture(GL_TEXTURE0 + CAGE_SHADER_TEXTURE_COLOR);
						f.eyeAdaptationKey = pass->eyeAdaptation.key;
						f.eyeAdaptationStrength = pass->eyeAdaptation.strength;
					}
					f.tonemap = pass->tonemap;
					f.tonemapEnabled = (pass->effects & cameraEffectsFlags::ToneMapping) == cameraEffectsFlags::ToneMapping;
					if ((pass->effects & cameraEffectsFlags::GammaCorrection) == cameraEffectsFlags::GammaCorrection)
						f.gamma = 1.0 / pass->gamma;
					else
						f.gamma = 1.0;
					finalScreenDataBuffer->bind();
					finalScreenDataBuffer->writeRange(&f, 0, sizeof(f));
					shaderFinalScreen->bind();
					renderEffect();
				}

				// fxaa
				if ((pass->effects & cameraEffectsFlags::AntiAliasing) == cameraEffectsFlags::AntiAliasing)
				{
					shaderFxaa->bind();
					renderEffect();
				}

				// blit to final output texture
				if (texSource != pass->targetTexture)
				{
					texTarget = pass->targetTexture;
					shaderBlit->bind();
					renderEffect();
				}
			}

			void renderShadowPass(renderPassStruct *pass)
			{
				renderTarget->bind();
				renderTarget->clear();
				renderTarget->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1].texture.get() : shadowmapsCube[-pass->targetShadowmap - 1].texture.get());
				renderTarget->activeAttachments(0);
				renderTarget->checkStatus();
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glClear(GL_DEPTH_BUFFER_BIT);
				CAGE_CHECK_GL_ERROR_DEBUG();

				shaderClass *shr = shaderDepth;
				shr->bind();
				for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
					renderObject(o, shr, o->mesh->getFlags() | meshFlags::DepthWrite);
				for (translucentStruct *o = pass->firstTranslucent; o; o = o->next)
					renderObject(&o->object, shr, o->object.mesh->getFlags() | meshFlags::DepthWrite);

				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderPass(renderPassStruct *pass)
			{
				viewportDataBuffer->bind();
				viewportDataBuffer->writeRange(&pass->shaderViewport, 0, sizeof(renderPassStruct::shaderViewportStruct));
				viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_SCISSOR_TEST);
				glDisable(GL_BLEND);
				lastTwoSided = true;
				setTwoSided(false);
				lastDepthTest = false; lastDepthWrite = false;
				setDepthTest(true, true);
				resetAllTextures();
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (pass->targetShadowmap == 0)
					renderCameraPass(pass);
				else
					renderShadowPass(pass);

				setTwoSided(false);
				setDepthTest(false, false);
				resetAllTextures();
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void gBufferResize(uint32 w, uint32 h)
			{
				if (w == gBufferWidth && h == gBufferHeight)
					return;
				gBufferWidth = w;
				gBufferHeight = h;
				albedoTexture->bind(); albedoTexture->image2d(w, h, GL_RGB8);
				specialTexture->bind(); specialTexture->image2d(w, h, GL_RG8);
				normalTexture->bind(); normalTexture->image2d(w, h, GL_RGB16F);
				colorTexture->bind(); colorTexture->image2d(w, h, GL_RGB16F);
				intermediateTexture->bind(); intermediateTexture->image2d(w, h, GL_RGB16F);
				velocityTexture->bind(); velocityTexture->image2d(w, h, GL_RG16F);
				ambientOcclusionTexture1->bind(); ambientOcclusionTexture1->image2d(max(w / CAGE_SHADER_SSAO_DOWNSCALE, 1u), max(h / CAGE_SHADER_SSAO_DOWNSCALE, 1u), GL_R8);
				ambientOcclusionTexture2->bind(); ambientOcclusionTexture2->image2d(max(w / CAGE_SHADER_SSAO_DOWNSCALE, 1u), max(h / CAGE_SHADER_SSAO_DOWNSCALE, 1u), GL_R8);
				bloomTexture1->bind(); bloomTexture1->image2d(max(w / CAGE_SHADER_BLOOM_DOWNSCALE, 1u), max(h / CAGE_SHADER_BLOOM_DOWNSCALE, 1u), GL_RGB16F);
				bloomTexture2->bind(); bloomTexture2->image2d(max(w / CAGE_SHADER_BLOOM_DOWNSCALE, 1u), max(h / CAGE_SHADER_BLOOM_DOWNSCALE, 1u), GL_RGB16F);
				depthTexture->bind(); depthTexture->image2d(w, h, GL_DEPTH_COMPONENT32);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

		public:
			graphicsDispatchImpl(const engineCreateConfig &config) : drawCalls(0), drawPrimitives(0), gBufferWidth(0), gBufferHeight(0), lastTwoSided(false), lastDepthTest(false)
			{
				detail::memset(static_cast<graphicsDispatchStruct*>(this), 0, sizeof(graphicsDispatchStruct));
			}

			void initialize()
			{
				shadowmaps2d.reserve(4);
				shadowmapsCube.reserve(32);

				gBufferWidth = gBufferHeight = 0;

#define GCHL_GENERATE(NAME) NAME = newTexture(); NAME->filters(GL_LINEAR, GL_LINEAR, 0); NAME->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, albedoTexture, specialTexture, normalTexture, colorTexture, intermediateTexture, velocityTexture, ambientOcclusionTexture1, ambientOcclusionTexture2, bloomTexture1, bloomTexture2, depthTexture));
#undef GCHL_GENERATE
				CAGE_CHECK_GL_ERROR_DEBUG();

				gBufferTarget = newDrawFrameBuffer();
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, velocityTexture.get());
				gBufferTarget->depthTexture(depthTexture.get());
				renderTarget = newDrawFrameBuffer();
				CAGE_CHECK_GL_ERROR_DEBUG();

				viewportDataBuffer = newUniformBuffer();
				viewportDataBuffer->writeWhole(nullptr, sizeof(renderPassStruct::shaderViewportStruct), GL_DYNAMIC_DRAW);
				meshDataBuffer = newUniformBuffer();
				meshDataBuffer->writeWhole(nullptr, sizeof(objectsStruct::shaderMeshStruct) * CAGE_SHADER_MAX_INSTANCES, GL_DYNAMIC_DRAW);
				armatureDataBuffer = newUniformBuffer();
				armatureDataBuffer->writeWhole(nullptr, sizeof(mat3x4) * CAGE_SHADER_MAX_BONES, GL_DYNAMIC_DRAW);
				lightsDataBuffer = newUniformBuffer();
				lightsDataBuffer->writeWhole(nullptr, sizeof(lightsStruct::shaderLightStruct) * CAGE_SHADER_MAX_INSTANCES, GL_DYNAMIC_DRAW);
				ssaoDataBuffer = newUniformBuffer();
				ssaoDataBuffer->writeWhole(nullptr, sizeof(ssaoShaderStruct), GL_DYNAMIC_DRAW);
				finalScreenDataBuffer = newUniformBuffer();
				finalScreenDataBuffer->writeWhole(nullptr, sizeof(finalScreenShaderStruct), GL_DYNAMIC_DRAW);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void finalize()
			{
				gBufferWidth = gBufferHeight = 0;
				*(graphicsDispatchHolders*)this = graphicsDispatchHolders();
			}

			void tick()
			{
				drawCalls = 0;
				drawPrimitives = 0;

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (windowWidth == 0 || windowHeight == 0)
					return;

				if (!shaderBlit)
				{
					glClear(GL_COLOR_BUFFER_BIT);
					CAGE_CHECK_GL_ERROR_DEBUG();
					return;
				}

				visualizableTextures.clear();
				visualizableTextures.emplace_back(colorTexture.get(), visualizableTextureModeEnum::Color); // unscaled
				visualizableTextures.emplace_back(colorTexture.get(), visualizableTextureModeEnum::Color); // scaled
				visualizableTextures.emplace_back(albedoTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(specialTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(normalTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(ambientOcclusionTexture1.get(), visualizableTextureModeEnum::Color);
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
				finalScreenDataBuffer->bind(CAGE_SHADER_UNIBLOCK_FINALSCREEN);
				ssaoDataBuffer->bind(CAGE_SHADER_UNIBLOCK_SSAO);
				CAGE_CHECK_GL_ERROR_DEBUG();

				{ // render all passes
					std::set<uintPtr> camerasToDestroy;
					for (auto &cs : cameras)
						camerasToDestroy.insert(cs.first);
					for (renderPassStruct *pass = firstRenderPass; pass; pass = pass->next)
					{
						renderPass(pass);
						CAGE_CHECK_GL_ERROR_DEBUG();
						camerasToDestroy.erase(pass->entityId);
					}
					for (auto r : camerasToDestroy)
						cameras.erase(r);
				}

				{ // blit to the window
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					viewportAndScissor(0, 0, windowWidth, windowHeight);
					setTwoSided(false);
					setDepthTest(false, false);
					sint32 visualizeIndex = visualizeBuffer % numeric_cast<sint32>(visualizableTextures.size());
					if (visualizeIndex < 0)
						visualizeIndex += numeric_cast<sint32>(visualizableTextures.size());
					meshSquare->bind();
					visualizableTextureStruct &v = visualizableTextures[visualizeIndex];
					v.tex->bind();
					if (visualizeIndex == 0)
					{
						CAGE_ASSERT_RUNTIME(v.visualizableTextureMode == visualizableTextureModeEnum::Color);
						shaderVisualizeColor->bind();
						shaderVisualizeColor->uniform(0, vec2(1.0 / gBufferWidth, 1.0 / gBufferHeight));
						renderDispatch(meshSquare, 1);
					}
					else
					{
						vec2 scale = vec2(1.0 / windowWidth, 1.0 / windowHeight);
						switch (v.visualizableTextureMode)
						{
						case visualizableTextureModeEnum::Color:
							shaderVisualizeColor->bind();
							shaderVisualizeColor->uniform(0, scale);
							renderDispatch(meshSquare, 1);
							break;
						case visualizableTextureModeEnum::Depth2d:
						case visualizableTextureModeEnum::DepthCube:
						{
							shaderVisualizeDepth->bind();
							shaderVisualizeDepth->uniform(0, scale);
							GLint cmpMode = 0;
							glGetTexParameteriv(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, &cmpMode);
							glTexParameteri(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, GL_NONE);
							renderDispatch(meshSquare, 1);
							glTexParameteri(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, cmpMode);
						} break;
						case visualizableTextureModeEnum::Velocity:
							shaderVisualizeVelocity->bind();
							shaderVisualizeVelocity->uniform(0, scale);
							renderDispatch(meshSquare, 1);
							break;
						}
					}
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
