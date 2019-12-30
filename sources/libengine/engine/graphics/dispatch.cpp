#include <vector>
#include <map>
#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/engine.h>
#include <cage-engine/opengl.h>
#include <cage-engine/graphics/shaderConventions.h>
#include <cage-engine/window.h>

#include "../engine.h"
#include "graphics.h"
#include "ssaoPoints.h"

namespace cage
{
	namespace
	{
		ConfigSint32 visualizeBuffer("cage/graphics/visualizeBuffer", 0);

		struct shadowmapBufferStruct
		{
		private:
			uint32 width;
			uint32 height;
		public:
			Holder<Texture> texture;
			void resize(uint32 w, uint32 h)
			{
				if (w == width && h == height)
					return;
				width = w;
				height = h;
				texture->bind();
				if (texture->getTarget() == GL_TEXTURE_CUBE_MAP)
					texture->imageCube(w, h, GL_DEPTH_COMPONENT16);
				else
					texture->image2d(w, h, GL_DEPTH_COMPONENT16);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
			shadowmapBufferStruct(uint32 target) : width(0), height(0)
			{
				CAGE_ASSERT(target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D);
				texture = newTexture(target);
				texture->setDebugName("shadowmap");
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
			Monochromatic,
			Velocity,
		};

		struct visualizableTextureStruct
		{
			Texture *tex;
			visualizableTextureModeEnum visualizableTextureMode;
			visualizableTextureStruct(Texture *tex, visualizableTextureModeEnum vtm) : tex(tex), visualizableTextureMode(vtm) {}
		};

		struct ssaoShaderStruct
		{
			mat4 viewProj;
			mat4 viewProjInv;
			vec4 params; // strength, bias, power, radius
			uint32 iparams[4]; // sampleCount, frameIndex
		};

		struct finalScreenShaderStruct
		{
			CameraTonemap tonemap; // 7 reals
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
			Holder<Texture> luminanceCollectionTexture; // w * h
			Holder<Texture> luminanceAccumulationTexture; // 1 * 1

			cameraSpecificDataStruct() : width(0), height(0), cameraEffects(CameraEffectsFlags::None)
			{}

			void update(uint32 w, uint32 h, CameraEffectsFlags ce)
			{
				if (w == width && h == height && ce == cameraEffects)
					return;
				width = w;
				height = h;
				cameraEffects = ce;

				if ((ce & CameraEffectsFlags::EyeAdaptation) == CameraEffectsFlags::EyeAdaptation)
				{
					if (luminanceCollectionTexture)
						luminanceCollectionTexture->bind();
					else
					{
						luminanceCollectionTexture = newTexture();
						luminanceCollectionTexture->setDebugName("luminanceCollectionTexture");
						luminanceCollectionTexture->filters(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0);
						luminanceCollectionTexture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					}
					luminanceCollectionTexture->image2d(max(w / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u), max(h / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u), GL_R16F);
					if (luminanceAccumulationTexture)
						luminanceAccumulationTexture->bind();
					else
					{
						luminanceAccumulationTexture = newTexture();
						luminanceAccumulationTexture->setDebugName("luminanceAccumulationTexture");
						luminanceAccumulationTexture->filters(GL_NEAREST, GL_NEAREST, 0);
						luminanceAccumulationTexture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					}
					luminanceAccumulationTexture->image2d(1, 1, GL_R16F);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				else
				{
					luminanceCollectionTexture.clear();
					luminanceAccumulationTexture.clear();
				}
			}

		private:
			uint32 width;
			uint32 height;
			CameraEffectsFlags cameraEffects;
		};

		struct uboCacheStruct
		{
			// double buffered ring buffer of uniform buffers :D
			std::vector<Holder<UniformBuffer>> data;
			uint32 current, last, prev;

			uboCacheStruct() : current(0), last(0), prev(0)
			{
				data.reserve(200);
				data.resize(10);
			}

			UniformBuffer *get()
			{
				if ((current + 1) % data.size() == prev)
				{
					// grow the buffer
					data.insert(data.begin() + prev, Holder<UniformBuffer>());
					prev++;
					if (last > current)
						last++;
				}
				auto &c = data[current];
				current = (current + 1) % data.size();
				if (!c)
				{
					c = newUniformBuffer({});
					c->setDebugName("uboCache");
				}
				return &*c;
			}

			void frame()
			{
				prev = last;
				last = current;
			}
		};

		struct graphicsDispatchHolders
		{
			Holder<FrameBuffer> gBufferTarget;
			Holder<FrameBuffer> renderTarget;
			Holder<Texture> albedoTexture;
			Holder<Texture> specialTexture;
			Holder<Texture> normalTexture;
			Holder<Texture> colorTexture;
			Holder<Texture> intermediateTexture;
			Holder<Texture> velocityTexture;
			Holder<Texture> ambientOcclusionTexture1;
			Holder<Texture> ambientOcclusionTexture2;
			Holder<Texture> bloomTexture1;
			Holder<Texture> bloomTexture2;
			Holder<Texture> depthTexture;

			Holder<UniformBuffer> ssaoPointsBuffer;
			uboCacheStruct uboCacheLarge;
			uboCacheStruct uboCacheSmall;

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
			uint32 frameIndex;
			uint32 lastGBufferWidth;
			uint32 lastGBufferHeight;
			CameraEffectsFlags lastCameraEffects;

			bool lastTwoSided;
			bool lastDepthTest;
			bool lastDepthWrite;
			
			Texture *texSource;
			Texture *texTarget;

			static void applyShaderRoutines(shaderConfigStruct *c, ShaderProgram *s)
			{
				s->uniform(CAGE_SHADER_UNI_ROUTINES, c->shaderRoutines, CAGE_SHADER_MAX_ROUTINES);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			static void viewportAndScissor(sint32 x, sint32 y, uint32 w, uint32 h)
			{
				glViewport(x, y, w, h);
				glScissor(x, y, w, h);
			}

			static void activeTexture(uint32 t)
			{
				glActiveTexture(GL_TEXTURE0 + t);
			}

			static void resetAllTextures()
			{
				for (uint32 i = 0; i < 16; i++)
				{
					activeTexture(i);
					glBindTexture(GL_TEXTURE_1D, 0);
					glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
					glBindTexture(GL_TEXTURE_3D, 0);
				}
				activeTexture(0);
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

			void useDisposableUbo(uint32 bindIndex, const void *data, uint32 size)
			{
				UniformBuffer *ubo = size > 256 ? uboCacheLarge.get() : uboCacheSmall.get();
				ubo->bind();
				if (ubo->getSize() < size)
					ubo->writeWhole(data, size, GL_DYNAMIC_DRAW);
				else
					ubo->writeRange(data, 0, size);
				ubo->bind(bindIndex);
			}

			template<class T>
			void useDisposableUbo(uint32 bindIndex, const T &data)
			{
				useDisposableUbo(bindIndex, &data, sizeof(T));
			}

			void bindGBufferTextures()
			{
				const uint32 tius[] = { CAGE_SHADER_TEXTURE_ALBEDO, CAGE_SHADER_TEXTURE_SPECIAL, CAGE_SHADER_TEXTURE_NORMAL, CAGE_SHADER_TEXTURE_COLOR, CAGE_SHADER_TEXTURE_DEPTH };
				static const uint32 cnt = sizeof(tius) / sizeof(tius[0]);
				Texture *texs[cnt] = { albedoTexture.get(), specialTexture.get(), normalTexture.get(), colorTexture.get(), depthTexture.get() };
				Texture::multiBind(cnt, tius, texs);
			}

			void bindShadowmap(sint32 shadowmap)
			{
				if (shadowmap != 0)
				{
					activeTexture((shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE));
					shadowmapBufferStruct &s = shadowmap > 0 ? shadowmaps2d[shadowmap - 1] : shadowmapsCube[-shadowmap - 1];
					s.texture->bind();
				}
			}

			void gaussianBlur(Holder<Texture> &texData, Holder<Texture> &texHelper, uint32 mipmapLevel = 0)
			{
				shaderGaussianBlur->bind();
				shaderGaussianBlur->uniform(1, (int)mipmapLevel);
				auto blur = [&](Holder<Texture> &tex1, Holder<Texture> &tex2, const vec2 &direction)
				{
					tex1->bind();
					renderTarget->colorTexture(0, tex2.get(), mipmapLevel);
#ifdef CAGE_DEBUG
					renderTarget->checkStatus();
#endif // CAGE_DEBUG
					shaderGaussianBlur->uniform(0, direction);
					renderDispatch(meshSquare, 1);
				};
				blur(texData, texHelper, vec2(1, 0));
				blur(texHelper, texData, vec2(0, 1));
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

			void renderDispatch(Mesh *mesh, uint32 count)
			{
				mesh->dispatch(count);
				drawCalls++;
				drawPrimitives += count * mesh->getPrimitivesCount();
			}

			void RenderObject(objectsStruct *obj, ShaderProgram *shr)
			{
				RenderObject(obj, shr, obj->mesh->getFlags());
			}

			void RenderObject(objectsStruct *obj, ShaderProgram *shr, MeshRenderFlags flags)
			{
				applyShaderRoutines(&obj->shaderConfig, shr);
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_MESHES, obj->shaderMeshes, sizeof(objectsStruct::shaderMeshStruct) * obj->count);
				if (obj->shaderArmatures)
				{
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_ARMATURES, obj->shaderArmatures, sizeof(mat3x4) * obj->count * obj->mesh->getSkeletonBones());
					shr->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, obj->mesh->getSkeletonBones());
				}
				obj->mesh->bind();
				setTwoSided(any(flags & MeshRenderFlags::TwoSided));
				setDepthTest(any(flags & MeshRenderFlags::DepthTest), any(flags & MeshRenderFlags::DepthWrite));
				{ // bind textures
					uint32 tius[MaxTexturesCountPerMaterial];
					Texture *texs[MaxTexturesCountPerMaterial];
					for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					{
						if (obj->textures[i])
						{
							switch (obj->textures[i]->getTarget())
							{
							case GL_TEXTURE_2D_ARRAY:
								tius[i] = CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i;
								break;
							case GL_TEXTURE_CUBE_MAP:
								tius[i] = CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i;
								break;
							default:
								tius[i] = CAGE_SHADER_TEXTURE_ALBEDO + i;
								break;
							}
							texs[i] = obj->textures[i];
						}
						else
						{
							tius[i] = 0;
							texs[i] = nullptr;
						}
					}
					Texture::multiBind(MaxTexturesCountPerMaterial, tius, texs);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
				renderDispatch(obj);
			}

			void renderOpaque(renderPassStruct *pass)
			{
				OPTICK_EVENT("opaque");
				ShaderProgram *shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				shr->bind();
				for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
					RenderObject(o, shr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderLighting(renderPassStruct *pass)
			{
				OPTICK_EVENT("lighting");
				ShaderProgram *shr = shaderLighting;
				shr->bind();
				for (lightsStruct *l = pass->firstLight; l; l = l->next)
				{
					applyShaderRoutines(&l->shaderConfig, shr);
					Mesh *mesh = nullptr;
					switch (l->lightType)
					{
					case LightTypeEnum::Directional:
						mesh = meshSquare;
						glCullFace(GL_BACK);
						break;
					case LightTypeEnum::Spot:
						mesh = meshCone;
						glCullFace(GL_FRONT);
						break;
					case LightTypeEnum::Point:
						mesh = meshSphere;
						glCullFace(GL_FRONT);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
					bindShadowmap(l->shadowmap);
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->shaderLights, sizeof(lightsStruct::shaderLightStruct) * l->count);
					mesh->bind();
					renderDispatch(mesh, l->count);
				}
				glCullFace(GL_BACK);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTranslucent(renderPassStruct *pass)
			{
				OPTICK_EVENT("translucent");
				ShaderProgram *shr = shaderTranslucent;
				shr->bind();
				for (translucentStruct *t = pass->firstTranslucent; t; t = t->next)
				{
					{ // render ambient object
						glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // assume premultiplied alpha
						uint32 tmp = CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE;
						std::swap(tmp, t->object.shaderConfig.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE]);
						RenderObject(&t->object, shr);
						std::swap(tmp, t->object.shaderConfig.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE]);
					}

					if (t->firstLight)
					{ // render lights on the object
						glBlendFunc(GL_ONE, GL_ONE); // assume premultiplied alpha
						for (lightsStruct *l = t->firstLight; l; l = l->next)
						{
							applyShaderRoutines(&l->shaderConfig, shr);
							bindShadowmap(l->shadowmap);
							useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->shaderLights, sizeof(lightsStruct::shaderLightStruct) * l->count);
							renderDispatch(t->object.mesh, l->count);
						}
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTexts(renderPassStruct *pass)
			{
				OPTICK_EVENT("texts");
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
				OPTICK_EVENT("camera pass");

				// camera specific data
				cameraSpecificDataStruct &cs = cameras[pass->entityId];
				cs.update(pass->vpW, pass->vpH, pass->effects);

				renderCameraOpaque(pass);
				renderCameraEffectsOpaque(pass, cs);
				renderCameraTransparencies(pass);
				renderCameraEffectsFinal(pass, cs);

				// blit to final output texture
				if (texSource != pass->targetTexture)
				{
					texTarget = pass->targetTexture;
					shaderBlit->bind();
					renderEffect();
				}
			}

			void renderCameraOpaque(renderPassStruct *pass)
			{
				OPTICK_EVENT("deferred");

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
				activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderCameraEffectsOpaque(renderPassStruct *pass, cameraSpecificDataStruct &cs)
			{
				OPTICK_EVENT("effects opaque");

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
				if (any(pass->effects & CameraEffectsFlags::AmbientOcclusion))
				{
					viewportAndScissor(pass->vpX / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpY / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpW / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpH / CAGE_SHADER_SSAO_DOWNSCALE);
					{
						ssaoShaderStruct s;
						s.viewProj = pass->viewProj;
						s.viewProjInv = inverse(pass->viewProj);
						s.params = vec4(pass->ssao.strength, pass->ssao.bias, pass->ssao.power, pass->ssao.worldRadius);
						s.iparams[0] = pass->ssao.samplesCount;
						s.iparams[1] = frameIndex;
						useDisposableUbo(CAGE_SHADER_UNIBLOCK_SSAO, s);
					}
					{ // generate
						renderTarget->colorTexture(0, ambientOcclusionTexture1.get());
						renderTarget->checkStatus();
						shaderSsaoGenerate->bind();
						renderDispatch(meshSquare, 1);
					}
					{ // blur
						for (uint32 i = 0; i < pass->ssao.blurPasses; i++)
							gaussianBlur(ambientOcclusionTexture1, ambientOcclusionTexture2);
					}
					{ // apply
						renderTarget->colorTexture(0, ambientOcclusionTexture2.get());
						renderTarget->checkStatus();
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
						ambientOcclusionTexture1->bind();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						shaderSsaoApply->bind();
						renderDispatch(meshSquare, 1);
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				}

				// ambient light
				if ((pass->shaderViewport.ambientLight + pass->shaderViewport.ambientDirectionalLight) != vec4())
				{
					shaderAmbient->bind();
					if (any(pass->effects & CameraEffectsFlags::AmbientOcclusion))
					{
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
						ambientOcclusionTexture2->bind();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						shaderAmbient->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 1);
					}
					else
						shaderAmbient->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 0);
					renderEffect();
				}

				// motion blur
				if (any(pass->effects & CameraEffectsFlags::MotionBlur))
				{
					activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
					velocityTexture->bind();
					activeTexture(CAGE_SHADER_TEXTURE_COLOR);
					shaderMotionBlur->bind();
					renderEffect();
				}

				// eye adaptation
				if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
				{
					// bind the luminance texture for use
					{
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
						cs.luminanceAccumulationTexture->bind();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
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
					texSource->bind();
				}
			}

			void renderCameraTransparencies(renderPassStruct *pass)
			{
				OPTICK_EVENT("transparencies");

				if (!!pass->firstTranslucent || !!pass->firstText)
				{
					if (texSource != colorTexture.get())
					{
						shaderBlit->bind();
						renderEffect();
					}
					CAGE_ASSERT(texSource == colorTexture.get());
					CAGE_ASSERT(texTarget == intermediateTexture.get());

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
					activeTexture(CAGE_SHADER_TEXTURE_COLOR);
					CAGE_CHECK_GL_ERROR_DEBUG();

					renderTarget->depthTexture(nullptr);
					renderTarget->activeAttachments(1);
					meshSquare->bind();
					bindGBufferTextures();
				}
			}

			void renderCameraEffectsFinal(renderPassStruct *pass, cameraSpecificDataStruct &cs)
			{
				OPTICK_EVENT("effects final");

				// bloom
				if (any(pass->effects & CameraEffectsFlags::Bloom))
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
						bloomTexture1->bind();
						bloomTexture1->generateMipmaps();
						for (uint32 i = 0; i < pass->bloom.blurPasses; i++)
						{
							uint32 d = CAGE_SHADER_BLOOM_DOWNSCALE + i;
							viewportAndScissor(pass->vpX / d, pass->vpY / d, pass->vpW / d, pass->vpH / d);
							gaussianBlur(bloomTexture1, bloomTexture2, i);
						}
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					{ // apply
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
						bloomTexture1->bind();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						CAGE_CHECK_GL_ERROR_DEBUG();
						shaderBloomApply->bind();
						shaderBloomApply->uniform(0, (int)pass->bloom.blurPasses);
						renderEffect();
					}
				}

				// final screen effects
				if (any(pass->effects & (CameraEffectsFlags::EyeAdaptation | CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					finalScreenShaderStruct f;
					if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
					{
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
						cs.luminanceAccumulationTexture->bind();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						f.eyeAdaptationKey = pass->eyeAdaptation.key;
						f.eyeAdaptationStrength = pass->eyeAdaptation.strength;
					}
					f.tonemap = pass->tonemap;
					f.tonemapEnabled = any(pass->effects & CameraEffectsFlags::ToneMapping);
					if (any(pass->effects & CameraEffectsFlags::GammaCorrection))
						f.gamma = 1.0 / pass->gamma;
					else
						f.gamma = 1.0;
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_FINALSCREEN, f);
					shaderFinalScreen->bind();
					renderEffect();
				}

				// fxaa
				if (any(pass->effects & CameraEffectsFlags::AntiAliasing))
				{
					shaderFxaa->bind();
					renderEffect();
				}
			}

			void renderShadowPass(renderPassStruct *pass)
			{
				OPTICK_EVENT("shadow pass");

				renderTarget->bind();
				renderTarget->clear();
				renderTarget->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1].texture.get() : shadowmapsCube[-pass->targetShadowmap - 1].texture.get());
				renderTarget->activeAttachments(0);
				renderTarget->checkStatus();
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glClear(GL_DEPTH_BUFFER_BIT);
				CAGE_CHECK_GL_ERROR_DEBUG();

				ShaderProgram *shr = shaderDepth;
				shr->bind();
				for (objectsStruct *o = pass->firstOpaque; o; o = o->next)
					RenderObject(o, shr, o->mesh->getFlags() | MeshRenderFlags::DepthWrite);
				for (translucentStruct *o = pass->firstTranslucent; o; o = o->next)
					RenderObject(&o->object, shr, o->object.mesh->getFlags() | MeshRenderFlags::DepthWrite);

				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderPass(renderPassStruct *pass)
			{
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_VIEWPORT, pass->shaderViewport);
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

			bool resizeTexture(const string &debugName, Holder<Texture> &texture, bool enabled, uint32 internalFormat, uint32 downscale = 1)
			{
				if (enabled)
				{
					if (texture)
						texture->bind();
					else
					{
						texture = newTexture();
						texture->setDebugName(debugName);
						texture->filters(GL_LINEAR, GL_LINEAR, 0);
						texture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					}
					texture->image2d(max(lastGBufferWidth / downscale, 1u), max(lastGBufferHeight / downscale, 1u), internalFormat);
				}
				else
					texture.clear();
				return enabled;
			}

			void gBufferResize(uint32 w, uint32 h, CameraEffectsFlags cameraEffects)
			{
				if (w == lastGBufferWidth && h == lastGBufferHeight && lastCameraEffects == cameraEffects)
					return;

				OPTICK_EVENT("g-buffer resize");

				lastGBufferWidth = w;
				lastGBufferHeight = h;
				lastCameraEffects = cameraEffects;

				albedoTexture->bind(); albedoTexture->image2d(w, h, GL_RGB8);
				specialTexture->bind(); specialTexture->image2d(w, h, GL_RG8);
				normalTexture->bind(); normalTexture->image2d(w, h, GL_RGB16F);
				colorTexture->bind(); colorTexture->image2d(w, h, GL_RGB16F);
				depthTexture->bind(); depthTexture->image2d(w, h, GL_DEPTH_COMPONENT32);
				intermediateTexture->bind(); intermediateTexture->image2d(w, h, GL_RGB16F);

				resizeTexture("velocityTexture", velocityTexture, any(cameraEffects & CameraEffectsFlags::MotionBlur), GL_RG16F);
				resizeTexture("ambientOcclusionTexture1", ambientOcclusionTexture1, any(cameraEffects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				resizeTexture("ambientOcclusionTexture2", ambientOcclusionTexture2, any(cameraEffects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				if (resizeTexture("bloomTexture1", bloomTexture1, any(cameraEffects & CameraEffectsFlags::Bloom), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE))
				{
					bloomTexture1->generateMipmaps();
					bloomTexture1->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
				}
				if (resizeTexture("bloomTexture2", bloomTexture2, any(cameraEffects & CameraEffectsFlags::Bloom), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE))
				{
					bloomTexture2->generateMipmaps();
					bloomTexture2->filters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();

				gBufferTarget->bind();
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, any(cameraEffects & CameraEffectsFlags::MotionBlur) ? velocityTexture.get() : nullptr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

		public:
			graphicsDispatchImpl(const EngineCreateConfig &config) : drawCalls(0), drawPrimitives(0), frameIndex(0), lastGBufferWidth(0), lastGBufferHeight(0), lastCameraEffects(CameraEffectsFlags::None), lastTwoSided(false), lastDepthTest(false)
			{
				detail::memset(static_cast<graphicsDispatchStruct*>(this), 0, sizeof(graphicsDispatchStruct));
			}

			void initialize()
			{
				shadowmaps2d.reserve(4);
				shadowmapsCube.reserve(32);

				lastGBufferWidth = lastGBufferHeight = 0;

#define GCHL_GENERATE(NAME) NAME = newTexture(); NAME->setDebugName(CAGE_STRINGIZE(NAME)); NAME->filters(GL_LINEAR, GL_LINEAR, 0); NAME->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, albedoTexture, specialTexture, normalTexture, colorTexture, depthTexture, intermediateTexture));
#undef GCHL_GENERATE
				CAGE_CHECK_GL_ERROR_DEBUG();

				gBufferTarget = newFrameBufferDraw();
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture.get());
				gBufferTarget->depthTexture(depthTexture.get());
				renderTarget = newFrameBufferDraw();
				CAGE_CHECK_GL_ERROR_DEBUG();

				ssaoPointsBuffer = newUniformBuffer();
				ssaoPointsBuffer->setDebugName("ssaoPointsBuffer");
				{
					const vec4 *points = nullptr;
					uint32 count = 0;
					pointsForSsaoShader(points, count);
					ssaoPointsBuffer->writeWhole((void*)points, count * sizeof(vec4), GL_STATIC_DRAW);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void finalize()
			{
				lastGBufferWidth = lastGBufferHeight = 0;
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
				if (windowWidth != lastGBufferWidth || windowHeight != lastGBufferHeight)
					visualizableTextures.emplace_back(colorTexture.get(), visualizableTextureModeEnum::Color); // scaled
				visualizableTextures.emplace_back(albedoTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(specialTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(normalTexture.get(), visualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(depthTexture.get(), visualizableTextureModeEnum::Depth2d);
				if (ambientOcclusionTexture2)
					visualizableTextures.emplace_back(ambientOcclusionTexture2.get(), visualizableTextureModeEnum::Monochromatic);
				if (bloomTexture1)
					visualizableTextures.emplace_back(bloomTexture1.get(), visualizableTextureModeEnum::Color);
				if (velocityTexture)
					visualizableTextures.emplace_back(velocityTexture.get(), visualizableTextureModeEnum::Velocity);

				{ // prepare all render targets
					uint32 maxW = windowWidth, maxH = windowHeight;
					CameraEffectsFlags cameraEffects = CameraEffectsFlags::None;
					for (renderPassStruct *renderPass = firstRenderPass; renderPass; renderPass = renderPass->next)
					{
						cameraEffects |= renderPass->effects;
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
					gBufferResize(maxW, maxH, cameraEffects);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				ssaoPointsBuffer->bind(CAGE_SHADER_UNIBLOCK_SSAO_POINTS);
				uboCacheLarge.frame();
				uboCacheSmall.frame();
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
						CAGE_ASSERT(v.visualizableTextureMode == visualizableTextureModeEnum::Color);
						shaderVisualizeColor->bind();
						shaderVisualizeColor->uniform(0, vec2(1.0 / lastGBufferWidth, 1.0 / lastGBufferHeight));
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
						case visualizableTextureModeEnum::Monochromatic:
							shaderVisualizeMonochromatic->bind();
							shaderVisualizeMonochromatic->uniform(0, scale);
							renderDispatch(meshSquare, 1);
							break;
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
					catch (const GraphicsError &)
					{
					}
				}

				frameIndex++;
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

	void graphicsDispatchCreate(const EngineCreateConfig &config)
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
