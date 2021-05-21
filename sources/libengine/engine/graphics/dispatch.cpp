#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/flatSet.h>

#include <cage-engine/graphicsError.h>
#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/window.h>
#include <cage-engine/provisionalRenderData.h>
#include <cage-engine/graphicsEffects.h>

#include "../engine.h"
#include "graphics.h"

#include <map>

namespace cage
{
	namespace
	{
		ConfigSint32 visualizeBuffer("cage/graphics/visualizeBuffer", 0);

		struct ShadowmapBuffer
		{
		private:
			uint32 width = 0;
			uint32 height = 0;
		public:
			Holder<Texture> texture;

			void resize(uint32 w, uint32 h)
			{
				if (w == width && h == height)
					return;
				width = w;
				height = h;
				texture->bind();
				if (texture->target() == GL_TEXTURE_CUBE_MAP)
					texture->imageCube(ivec2(w, h), GL_DEPTH_COMPONENT16);
				else
					texture->image2d(ivec2(w, h), GL_DEPTH_COMPONENT24);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			ShadowmapBuffer(uint32 target)
			{
				CAGE_ASSERT(target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D);
				texture = newTexture(target);
				texture->setDebugName("shadowmap");
				texture->filters(GL_LINEAR, GL_LINEAR, 16);
				texture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}
		};

		enum class VisualizableTextureModeEnum
		{
			Color,
			Depth2d,
			DepthCube,
			Monochromatic,
			Velocity,
		};

		struct VisualizableTexture
		{
			Texture *tex = nullptr;
			VisualizableTextureModeEnum visualizableTextureMode;

			VisualizableTexture(Texture *tex, VisualizableTextureModeEnum vtm) : tex(tex), visualizableTextureMode(vtm)
			{}
		};

		struct FinalScreenShader
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

		struct CameraSpecificData
		{
			Holder<Texture> luminanceCollectionTexture; // w * h
			Holder<Texture> luminanceAccumulationTexture; // 1 * 1

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
					luminanceCollectionTexture->image2d(max(ivec2(w, h) / CAGE_SHADER_LUMINANCE_DOWNSCALE, 1u), GL_R16F);
					if (luminanceAccumulationTexture)
						luminanceAccumulationTexture->bind();
					else
					{
						luminanceAccumulationTexture = newTexture();
						luminanceAccumulationTexture->setDebugName("luminanceAccumulationTexture");
						luminanceAccumulationTexture->filters(GL_NEAREST, GL_NEAREST, 0);
						luminanceAccumulationTexture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					}
					luminanceAccumulationTexture->image2d(ivec2(1), GL_R16F);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}
				else
				{
					luminanceCollectionTexture.clear();
					luminanceAccumulationTexture.clear();
				}
			}

		private:
			uint32 width = 0;
			uint32 height = 0;
			CameraEffectsFlags cameraEffects = CameraEffectsFlags::None;
		};

		struct GraphicsDispatchHolders
		{
			Holder<Model> modelSquare, modelSphere, modelCone;
			Holder<ShaderProgram> shaderVisualizeColor, shaderVisualizeDepth, shaderVisualizeMonochromatic, shaderVisualizeVelocity;
			Holder<ShaderProgram> shaderAmbient, shaderBlit, shaderDepth, shaderGBuffer, shaderLighting, shaderTranslucent;
			Holder<ShaderProgram> shaderGaussianBlur, shaderMotionBlur, shaderBloomGenerate, shaderBloomApply, shaderLuminanceCollection, shaderLuminanceCopy, shaderFinalScreen, shaderFxaa;
			Holder<ShaderProgram> shaderFont;

			Holder<FrameBuffer> gBufferTarget;
			Holder<FrameBuffer> renderTarget;
			Holder<Texture> albedoTexture;
			Holder<Texture> specialTexture;
			Holder<Texture> normalTexture;
			Holder<Texture> colorTexture;
			Holder<Texture> intermediateTexture;
			Holder<Texture> velocityTexture;
			Holder<Texture> ambientOcclusionTexture;
			Holder<Texture> bloomTexture1;
			Holder<Texture> bloomTexture2;
			Holder<Texture> depthTexture;

			Holder<RenderQueue> renderQueue;
			Holder<ProvisionalRenderData> provisionalData;

			Holder<Texture> texSource;
			Holder<Texture> texTarget;

			std::vector<ShadowmapBuffer> shadowmaps2d, shadowmapsCube;
			std::vector<VisualizableTexture> visualizableTextures;
			std::map<uintPtr, CameraSpecificData> cameras;
		};

		struct GraphicsDispatchImpl : public GraphicsDispatch, private GraphicsDispatchHolders
		{
		public:
			uint32 drawCalls = 0;
			uint32 drawPrimitives = 0;

		private:
			uint32 frameIndex = 0;
			uint32 lastGBufferWidth = 0;
			uint32 lastGBufferHeight = 0;
			CameraEffectsFlags lastCameraEffects = CameraEffectsFlags::None;
			GfCommonConfig gfCommonConfig; // helper to simplify initialization

			void applyShaderRoutines(const ShaderConfig *c, const Holder<ShaderProgram> &s)
			{
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, c->shaderRoutines);
				renderQueue->checkGlErrorDebug();
			}

			void viewportAndScissor(sint32 x, sint32 y, uint32 w, uint32 h)
			{
				renderQueue->viewport(ivec2(x, y), ivec2(w, h));
			}

			void resetAllTextures()
			{
				renderQueue->resetAllTextures();
			}

			void setTwoSided(bool twoSided)
			{
				renderQueue->culling(!twoSided);
			}

			void setDepthTest(bool depthTest, bool depthWrite)
			{
				renderQueue->depthTest(depthTest);
				renderQueue->depthWrite(depthWrite);
			}

			void useDisposableUbo(uint32 bindIndex, PointerRange<const char> data)
			{
				renderQueue->universalUniformBuffer(data, bindIndex);
			}

			template<class T>
			void useDisposableUbo(uint32 bindIndex, const T &data)
			{
				renderQueue->universalUniformStruct<T>(data, bindIndex);
			}

			template<class T>
			void useDisposableUbo(uint32 bindIndex, const std::vector<T> &data)
			{
				renderQueue->universalUniformArray<T>(data, bindIndex);
			}

			void bindGBufferTextures()
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("bind gBuffer textures");
				renderQueue->bind(albedoTexture, CAGE_SHADER_TEXTURE_ALBEDO);
				renderQueue->bind(specialTexture, CAGE_SHADER_TEXTURE_SPECIAL);
				renderQueue->bind(normalTexture, CAGE_SHADER_TEXTURE_NORMAL);
				renderQueue->bind(colorTexture, CAGE_SHADER_TEXTURE_COLOR);
				renderQueue->bind(depthTexture, CAGE_SHADER_TEXTURE_DEPTH);
				renderQueue->activeTexture(4);
			}

			void bindShadowmap(sint32 shadowmap)
			{
				if (shadowmap != 0)
				{
					ShadowmapBuffer &s = shadowmap > 0 ? shadowmaps2d[shadowmap - 1] : shadowmapsCube[-shadowmap - 1];
					renderQueue->bind(s.texture, shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE);
				}
			}

			void gaussianBlur(const Holder<Texture> &texData, const Holder<Texture> &texHelper, uint32 mipmapLevel = 0)
			{
				renderQueue->bind(shaderGaussianBlur);
				renderQueue->uniform(1, (int)mipmapLevel);
				auto blur = [&](const Holder<Texture> &tex1, const Holder<Texture> &tex2, const vec2 &direction)
				{
					renderQueue->bind(tex1);
					renderQueue->colorTexture(0, tex2, mipmapLevel);
#ifdef CAGE_DEBUG
					renderQueue->checkFrameBuffer();
#endif // CAGE_DEBUG
					renderQueue->uniform(0, direction);
					renderDispatch(modelSquare, 1);
				};
				blur(texData, texHelper, vec2(1, 0));
				blur(texHelper, texData, vec2(0, 1));
			}

			void renderEffect()
			{
				renderQueue->checkGlErrorDebug();
				renderQueue->colorTexture(0, texTarget);
#ifdef CAGE_DEBUG
				renderQueue->checkFrameBuffer();
#endif // CAGE_DEBUG
				renderQueue->bind(texSource);
				renderDispatch(modelSquare, 1);
				std::swap(texSource, texTarget);
			}

			void renderDispatch(const Objects *obj)
			{
				renderDispatch(obj->model, numeric_cast<uint32>(obj->uniModeles.size()));
			}

			void renderDispatch(const Holder<Model> &model, uint32 count)
			{
				renderQueue->draw(count);
			}

			void renderObject(const Objects *obj, const Holder<ShaderProgram> &shr)
			{
				renderObject(obj, shr, obj->model->flags);
			}

			void renderObject(const Objects *obj, const Holder<ShaderProgram> &shr, ModelRenderFlags flags)
			{
				applyShaderRoutines(&obj->shaderConfig, shr);
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_MESHES, obj->uniModeles);
				if (!obj->uniArmatures.empty())
				{
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_ARMATURES, obj->uniArmatures);
					renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, obj->model->skeletonBones);
				}
				renderQueue->bind(obj->model);
				setTwoSided(any(flags & ModelRenderFlags::TwoSided));
				setDepthTest(any(flags & ModelRenderFlags::DepthTest), any(flags & ModelRenderFlags::DepthWrite));
				{ // bind textures
					for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					{
						if (obj->textures[i])
						{
							switch (obj->textures[i]->target())
							{
							case GL_TEXTURE_2D_ARRAY:
								renderQueue->bind(obj->textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i);
								break;
							case GL_TEXTURE_CUBE_MAP:
								renderQueue->bind(obj->textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i);
								break;
							default:
								renderQueue->bind(obj->textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i);
								break;
							}
						}
					}
					renderQueue->activeTexture(0);
				}
				renderQueue->checkGlErrorDebug();
				renderDispatch(obj);
			}

			void renderOpaque(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("opaque");
				OPTICK_EVENT("opaque");
				const Holder<ShaderProgram> &shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				renderQueue->bind(shr);
				for (const Holder<Objects> &o : pass->opaques)
					renderObject(o.get(), shr);
				renderQueue->checkGlErrorDebug();
			}

			void renderLighting(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("lighting");
				OPTICK_EVENT("lighting");
				const Holder<ShaderProgram> &shr = shaderLighting;
				renderQueue->bind(shr);
				for (const Holder<Lights> &l : pass->lights)
				{
					applyShaderRoutines(&l->shaderConfig, shr);
					Holder<Model> model;
					switch (l->lightType)
					{
					case LightTypeEnum::Directional:
						model = modelSquare.share();
						renderQueue->cullFace(false);
						break;
					case LightTypeEnum::Spot:
						model = modelCone.share();
						renderQueue->cullFace(true);
						break;
					case LightTypeEnum::Point:
						model = modelSphere.share();
						renderQueue->cullFace(true);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
					bindShadowmap(l->shadowmap);
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->uniLights);
					renderQueue->bind(model);
					renderDispatch(model, numeric_cast<uint32>(l->uniLights.size()));
				}
				renderQueue->cullFace(false);
				renderQueue->checkGlErrorDebug();
			}

			void renderTranslucent(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("translucent");
				OPTICK_EVENT("translucent");
				const Holder<ShaderProgram> &shr = shaderTranslucent;
				renderQueue->bind(shr);
				for (const Holder<Translucent> &t : pass->translucents)
				{
					{ // render ambient object
						renderQueue->blendFuncPremultipliedTransparency();
						uint32 tmp = CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE;
						uint32 &orig = const_cast<uint32&>(t->object.shaderConfig.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE]);
						std::swap(tmp, orig);
						renderObject(&t->object, shr);
						std::swap(tmp, orig);
					}

					if (!t->lights.empty())
					{ // render lights on the object
						renderQueue->blendFuncAdditive();
						for (const Holder<Lights> &l : t->lights)
						{
							applyShaderRoutines(&l->shaderConfig, shr);
							bindShadowmap(l->shadowmap);
							useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->uniLights);
							renderDispatch(t->object.model, numeric_cast<uint32>(l->uniLights.size()));
						}
					}
				}
				renderQueue->checkGlErrorDebug();
			}

			void renderTexts(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("texts");
				OPTICK_EVENT("texts");
				for (const Holder<Texts> &t : pass->texts)
				{
					renderQueue->bind(shaderFont);
					for (const Holder<Texts::Render> &r : t->renders)
					{
						renderQueue->uniform(0, r->transform);
						renderQueue->uniform(4, r->color);
						t->font->bind(+renderQueue, modelSquare, shaderFont);
						t->font->render(+renderQueue, r->glyphs, r->format);
					}
				}
				renderQueue->checkGlErrorDebug();
			}

			void renderCameraPass(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("camera pass");
				OPTICK_EVENT("camera pass");

				// camera specific data
				CAGE_ASSERT(pass->entityId != 0);
				CameraSpecificData &cs = cameras[pass->entityId];
				cs.update(pass->vpW, pass->vpH, pass->effects);

				renderCameraOpaque(pass);
				renderCameraEffectsOpaque(pass, cs);
				renderCameraTransparencies(pass);
				renderCameraEffectsFinal(pass, cs);

				// blit to final output texture
				CAGE_ASSERT(pass->targetTexture);
				if (+texSource != pass->targetTexture)
				{
					texTarget = Holder<Texture>(pass->targetTexture, nullptr); // non-owning holder - temporary fix
					renderQueue->bind(shaderBlit);
					renderEffect();
				}
			}

			void renderCameraOpaque(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("deferred");
				OPTICK_EVENT("deferred");

				// opaque
				renderQueue->bind(gBufferTarget);
				renderQueue->activeAttachments(31);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();
				if (pass->clearFlags)
					renderQueue->clear(pass->clearFlags & GL_COLOR_BUFFER_BIT, pass->clearFlags & GL_DEPTH_BUFFER_BIT, pass->clearFlags & GL_STENCIL_BUFFER_BIT);
				renderOpaque(pass);
				setTwoSided(false);
				setDepthTest(false, false);
				renderQueue->checkGlErrorDebug();

				// lighting
				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->colorTexture(0, colorTexture);
				renderQueue->activeAttachments(1);
				renderQueue->checkFrameBuffer();
				bindGBufferTextures();
				renderQueue->blending(true);
				renderQueue->blendFuncAdditive();
				renderQueue->checkGlErrorDebug();
				renderLighting(pass);
				setDepthTest(false, false);
				renderQueue->blending(false);
				renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				renderQueue->checkGlErrorDebug();
			}

			void renderCameraEffectsOpaque(const RenderPass *pass, CameraSpecificData &cs)
			{
				const auto graphicsDebugScope2 = renderQueue->scopedNamedPass("effects opaque");
				OPTICK_EVENT("effects opaque");

				// opaque screen-space effects
				renderQueue->bind(renderTarget);
				renderQueue->depthTexture(Holder<Texture>());
				renderQueue->activeAttachments(1);
				bindGBufferTextures();
				renderQueue->bind(modelSquare);
				texSource = colorTexture.share();
				texTarget = intermediateTexture.share();
				renderQueue->checkGlErrorDebug();

				gfCommonConfig.resolution = ivec2(pass->vpW, pass->vpH);

				// ssao
				if (any(pass->effects & CameraEffectsFlags::AmbientOcclusion))
				{
					GfSsaoConfig cfg;
					(GfCommonConfig &)cfg = gfCommonConfig;
					(GfSsao &)cfg = pass->ssao;
					cfg.frameIndex = frameIndex;
					cfg.inDepth = depthTexture;
					cfg.inNormal = normalTexture;
					cfg.outAo = ambientOcclusionTexture;
					cfg.viewProj = pass->viewProj;
					gfSsao(cfg);

					renderQueue->bind(renderTarget);
					renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				}

				// ambient light
				if ((pass->uniViewport.ambientLight + pass->uniViewport.ambientDirectionalLight) != vec4())
				{
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("ambient light");
					renderQueue->bind(shaderAmbient);
					if (any(pass->effects & CameraEffectsFlags::AmbientOcclusion))
					{
						renderQueue->bind(ambientOcclusionTexture, CAGE_SHADER_TEXTURE_EFFECTS);
						renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 1);
					}
					else
						renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 0);
					renderEffect();
				}

				// depth of field
				if (any(pass->effects & CameraEffectsFlags::DepthOfField))
				{
					GfDepthOfFieldConfig cfg;
					(GfCommonConfig &)cfg = gfCommonConfig;
					(GfDepthOfField &)cfg = pass->depthOfField;
					cfg.proj = pass->proj;
					cfg.inDepth = depthTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					gfDepthOfField(cfg);
					std::swap(texSource, texTarget);

					renderQueue->bind(renderTarget);
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				}

				// motion blur
				if (any(pass->effects & CameraEffectsFlags::MotionBlur))
				{
					GfMotionBlurConfig cfg;
					(GfCommonConfig &)cfg = gfCommonConfig;
					(GfMotionBlur &)cfg = pass->motionBlur;
					cfg.inVelocity = velocityTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					gfMotionBlur(cfg);
					std::swap(texSource, texTarget);

					renderQueue->bind(renderTarget);
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				}

				// eye adaptation
				if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
				{
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("eye adaptation");
					// bind the luminance texture for use
					{
						renderQueue->bind(cs.luminanceAccumulationTexture, CAGE_SHADER_TEXTURE_EFFECTS);
						renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
					}
					// luminance collection
					{
						viewportAndScissor(pass->vpX / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpY / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpW / CAGE_SHADER_LUMINANCE_DOWNSCALE, pass->vpH / CAGE_SHADER_LUMINANCE_DOWNSCALE);
						renderQueue->colorTexture(0, cs.luminanceCollectionTexture);
						renderQueue->checkFrameBuffer();
						renderQueue->bind(texSource);
						renderQueue->bind(shaderLuminanceCollection);
						renderDispatch(modelSquare, 1);
					}
					// downscale
					{
						renderQueue->bind(cs.luminanceCollectionTexture);
						renderQueue->generateMipmaps();
					}
					// luminance copy
					{
						viewportAndScissor(0, 0, 1, 1);
						renderQueue->colorTexture(0, cs.luminanceAccumulationTexture);
						renderQueue->checkFrameBuffer();
						renderQueue->bind(shaderLuminanceCopy);
						renderQueue->uniform(0, vec2(pass->eyeAdaptation.darkerSpeed, pass->eyeAdaptation.lighterSpeed));
						renderDispatch(modelSquare, 1);
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					renderQueue->bind(texSource);
				}
			}

			void renderCameraTransparencies(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("transparencies");
				OPTICK_EVENT("transparencies");

				if (pass->translucents.empty() && pass->texts.empty())
					return;

				if (+texSource != +colorTexture)
				{
					renderQueue->bind(shaderBlit);
					renderEffect();
				}
				CAGE_ASSERT(+texSource == +colorTexture);
				CAGE_ASSERT(+texTarget == +intermediateTexture);

				renderQueue->colorTexture(0, colorTexture);
				renderQueue->depthTexture(depthTexture);
				renderQueue->activeAttachments(1);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();

				setDepthTest(true, true);
				renderQueue->blending(true);
				renderQueue->checkGlErrorDebug();

				renderTranslucent(pass);

				setDepthTest(true, false);
				setTwoSided(true);
				renderQueue->blendFuncAlphaTransparency();
				renderQueue->checkGlErrorDebug();

				renderTexts(pass);

				setDepthTest(false, false);
				setTwoSided(false);
				renderQueue->blending(false);
				renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				renderQueue->checkGlErrorDebug();

				renderQueue->depthTexture(Holder<Texture>());
				renderQueue->activeAttachments(1);
				renderQueue->bind(modelSquare);
				bindGBufferTextures();
			}

			void renderCameraEffectsFinal(const RenderPass *pass, CameraSpecificData &cs)
			{
				const auto graphicsDebugScope2 = renderQueue->scopedNamedPass("effects final");
				OPTICK_EVENT("effects final");

				// bloom
				if (any(pass->effects & CameraEffectsFlags::Bloom))
				{
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("bloom");
					viewportAndScissor(pass->vpX / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpY / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpW / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpH / CAGE_SHADER_BLOOM_DOWNSCALE);
					{ // generate
						renderQueue->colorTexture(0, bloomTexture1);
						renderQueue->checkFrameBuffer();
						renderQueue->bind(shaderBloomGenerate);
						renderQueue->uniform(0, vec4(pass->bloom.threshold, 0, 0, 0));
						renderDispatch(modelSquare, 1);
					}
					{ // blur
						renderQueue->bind(bloomTexture1);
						renderQueue->generateMipmaps();
						for (uint32 i = 0; i < pass->bloom.blurPasses; i++)
						{
							uint32 d = CAGE_SHADER_BLOOM_DOWNSCALE + i;
							viewportAndScissor(pass->vpX / d, pass->vpY / d, pass->vpW / d, pass->vpH / d);
							gaussianBlur(bloomTexture1, bloomTexture2, i);
						}
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					{ // apply
						renderQueue->bind(bloomTexture1, CAGE_SHADER_TEXTURE_EFFECTS);
						renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->bind(shaderBloomApply);
						renderQueue->uniform(0, (int)pass->bloom.blurPasses);
						renderEffect();
					}
				}

				// final screen effects
				if (any(pass->effects & (CameraEffectsFlags::EyeAdaptation | CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("final screen");
					FinalScreenShader f;
					if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
					{
						renderQueue->bind(cs.luminanceAccumulationTexture, CAGE_SHADER_TEXTURE_EFFECTS);
						renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						f.eyeAdaptationKey = pass->eyeAdaptation.key;
						f.eyeAdaptationStrength = pass->eyeAdaptation.strength;
					}
					f.tonemap = pass->tonemap;
					f.tonemapEnabled = any(pass->effects & CameraEffectsFlags::ToneMapping);
					if (any(pass->effects & CameraEffectsFlags::GammaCorrection))
						f.gamma = 1.0 / pass->gamma;
					else
						f.gamma = 1.0;
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES, f);
					renderQueue->bind(shaderFinalScreen);
					renderEffect();
				}

				// fxaa
				if (any(pass->effects & CameraEffectsFlags::AntiAliasing))
				{
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("fxaa");
					renderQueue->bind(shaderFxaa);
					renderEffect();
				}
			}

			void renderShadowPass(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->scopedNamedPass("shadow pass");
				OPTICK_EVENT("shadow pass");

				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1].texture : shadowmapsCube[-pass->targetShadowmap - 1].texture);
				renderQueue->activeAttachments(0);
				renderQueue->checkFrameBuffer();
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->checkGlErrorDebug();

				const Holder<ShaderProgram> &shr = shaderDepth;
				renderQueue->bind(shr);
				for (const Holder<Objects> &o : pass->opaques)
					renderObject(o.get(), shr, o->model->flags | ModelRenderFlags::DepthWrite);
				for (const Holder<Translucent> &o : pass->translucents)
					renderObject(&o->object, shr, o->object.model->flags | ModelRenderFlags::DepthWrite);

				renderQueue->colorWrite(true);
				renderQueue->checkGlErrorDebug();
			}

			void renderPass(const RenderPass *pass)
			{
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_VIEWPORT, pass->uniViewport);
				viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				renderQueue->depthTest(true);
				renderQueue->scissors(true);
				renderQueue->blending(false);
				setTwoSided(false);
				setDepthTest(true, true);
				resetAllTextures();
				renderQueue->checkGlErrorDebug();

				if (pass->targetShadowmap == 0)
					renderCameraPass(pass);
				else
					renderShadowPass(pass);

				setTwoSided(false);
				setDepthTest(false, false);
				resetAllTextures();
				renderQueue->checkGlErrorDebug();
			}

			bool resizeTexture(const char *debugName, Holder<Texture> &texture, bool enabled, uint32 internalFormat, uint32 downscale = 1, bool mipmaps = false)
			{
				if (enabled)
				{
					if (texture)
						texture->bind();
					else
					{
						texture = newTexture();
						texture->setDebugName(debugName);
						texture->filters(mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR, GL_LINEAR, 0);
						texture->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					}
					texture->image2d(max(ivec2(lastGBufferWidth, lastGBufferHeight) / downscale, 1u), internalFormat);
					if (mipmaps)
						texture->generateMipmaps();
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

				resizeTexture("albedoTexture", albedoTexture, true, GL_RGB8);
				resizeTexture("specialTexture", specialTexture, true, GL_RG8);
				resizeTexture("normalTexture", normalTexture, true, GL_RGB16F);
				resizeTexture("colorTexture", colorTexture, true, GL_RGB16F);
				resizeTexture("depthTexture", depthTexture, true, GL_DEPTH_COMPONENT32);
				resizeTexture("intermediateTexture", intermediateTexture, true, GL_RGB16F);
				resizeTexture("velocityTexture", velocityTexture, any(cameraEffects & CameraEffectsFlags::MotionBlur), GL_RG16F);
				resizeTexture("ambientOcclusionTexture", ambientOcclusionTexture, any(cameraEffects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				resizeTexture("bloomTexture1", bloomTexture1, any(cameraEffects & CameraEffectsFlags::Bloom), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE, true);
				resizeTexture("bloomTexture2", bloomTexture2, any(cameraEffects & CameraEffectsFlags::Bloom), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE, true);
				CAGE_CHECK_GL_ERROR_DEBUG();

				gBufferTarget->bind();
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture.get());
				gBufferTarget->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, any(cameraEffects & CameraEffectsFlags::MotionBlur) ? velocityTexture.get() : nullptr);
				gBufferTarget->depthTexture(depthTexture.get());
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			struct DepthMapVisualizationData
			{
				Holder<Model> modelSquare;
				Texture *tex = nullptr;
			};

			static void depthMapVisualizationFunction(void *ptr)
			{
				const DepthMapVisualizationData *data = (const DepthMapVisualizationData *)ptr;
				GLint cmpMode = 0;
				glGetTexParameteriv(data->tex->target(), GL_TEXTURE_COMPARE_MODE, &cmpMode);
				glTexParameteri(data->tex->target(), GL_TEXTURE_COMPARE_MODE, GL_NONE);
				data->modelSquare->dispatch();
				glTexParameteri(data->tex->target(), GL_TEXTURE_COMPARE_MODE, cmpMode);
			}

		public:
			explicit GraphicsDispatchImpl(const EngineCreateConfig &config)
			{}

			void initialize()
			{
				shadowmaps2d.reserve(4);
				shadowmapsCube.reserve(32);

				gBufferTarget = newFrameBufferDraw();
				renderTarget = newFrameBufferDraw();
				CAGE_CHECK_GL_ERROR_DEBUG();

				renderQueue = newRenderQueue();
				provisionalData = newProvisionalRenderData();
			}

			void finalize()
			{
				lastGBufferWidth = lastGBufferHeight = 0;
				*(GraphicsDispatchHolders*)this = GraphicsDispatchHolders();
			}

			void tick()
			{
				GraphicsDebugScope graphicsDebugScope("engine tick");

				drawCalls = 0;
				drawPrimitives = 0;

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (windowWidth == 0 || windowHeight == 0)
					return;

				AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")))
					return;

				modelSquare = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelSphere = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/sphere.obj"));
				modelCone = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/cone.obj"));
				shaderVisualizeColor = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/color.glsl"));
				shaderVisualizeDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/depth.glsl"));
				shaderVisualizeMonochromatic = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/monochromatic.glsl"));
				shaderVisualizeVelocity = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/velocity.glsl"));
				shaderAmbient = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/ambient.glsl"));
				shaderBlit = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
				shaderDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/depth.glsl"));
				shaderGBuffer = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/gBuffer.glsl"));
				shaderLighting = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/lighting.glsl"));
				shaderTranslucent = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/translucent.glsl"));
				shaderGaussianBlur = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/gaussianBlur.glsl"));
				shaderMotionBlur = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/motionBlur.glsl"));
				shaderBloomGenerate = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomGenerate.glsl"));
				shaderBloomApply = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomApply.glsl"));
				shaderLuminanceCollection = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCollection.glsl"));
				shaderLuminanceCopy = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCopy.glsl"));
				shaderFinalScreen = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/finalScreen.glsl"));
				shaderFxaa = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/fxaa.glsl"));
				shaderFont = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));

				if (!shaderBlit)
					return;

				visualizableTextures.clear();
				visualizableTextures.emplace_back(colorTexture.get(), VisualizableTextureModeEnum::Color); // unscaled
				if (windowWidth != lastGBufferWidth || windowHeight != lastGBufferHeight)
					visualizableTextures.emplace_back(colorTexture.get(), VisualizableTextureModeEnum::Color); // scaled
				visualizableTextures.emplace_back(albedoTexture.get(), VisualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(specialTexture.get(), VisualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(normalTexture.get(), VisualizableTextureModeEnum::Color);
				visualizableTextures.emplace_back(depthTexture.get(), VisualizableTextureModeEnum::Depth2d);
				if (ambientOcclusionTexture)
					visualizableTextures.emplace_back(ambientOcclusionTexture.get(), VisualizableTextureModeEnum::Monochromatic);
				if (bloomTexture1)
					visualizableTextures.emplace_back(bloomTexture1.get(), VisualizableTextureModeEnum::Color);
				if (velocityTexture)
					visualizableTextures.emplace_back(velocityTexture.get(), VisualizableTextureModeEnum::Velocity);

				{ // prepare all render targets
					uint32 maxW = windowWidth, maxH = windowHeight;
					CameraEffectsFlags cameraEffects = CameraEffectsFlags::None;
					for (const Holder<RenderPass> &renderPass : renderPasses)
					{
						cameraEffects |= renderPass->effects;
						if (renderPass->targetTexture)
						{ // render to texture
							visualizableTextures.emplace_back(renderPass->targetTexture, VisualizableTextureModeEnum::Color);
							const ivec2 res = renderPass->targetTexture->resolution();
							maxW = max(maxW, numeric_cast<uint32>(res[0]));
							maxH = max(maxH, numeric_cast<uint32>(res[1]));
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
								shadowmaps2d.push_back(ShadowmapBuffer(GL_TEXTURE_2D));
							ShadowmapBuffer &s = shadowmaps2d[idx];
							s.resize(renderPass->shadowmapResolution, renderPass->shadowmapResolution);
							visualizableTextures.emplace_back(s.texture.get(), VisualizableTextureModeEnum::Depth2d);
						}
						if (renderPass->targetShadowmap < 0)
						{ // cube shadowmap
							uint32 idx = -renderPass->targetShadowmap - 1;
							while (shadowmapsCube.size() <= idx)
								shadowmapsCube.push_back(ShadowmapBuffer(GL_TEXTURE_CUBE_MAP));
							ShadowmapBuffer &s = shadowmapsCube[idx];
							s.resize(renderPass->shadowmapResolution, renderPass->shadowmapResolution);
							visualizableTextures.emplace_back(s.texture.get(), VisualizableTextureModeEnum::DepthCube);
						}
					}
					gBufferResize(maxW, maxH, cameraEffects);
					CAGE_CHECK_GL_ERROR_DEBUG();
				}

				if (!visualizableTextures[0].tex)
					return;

				renderQueue->resetQueue();
				provisionalData->reset();

				gfCommonConfig.assets = engineAssets();
				gfCommonConfig.provisionals = +provisionalData;
				gfCommonConfig.queue = +renderQueue;

				{ // render all passes
					FlatSet<uintPtr> camerasToDestroy;
					for (auto &cs : cameras)
						camerasToDestroy.insert(cs.first);
					for (const Holder<RenderPass> &pass : renderPasses)
					{
						renderPass(pass.get());
						renderQueue->checkGlErrorDebug();
						camerasToDestroy.erase(pass->entityId);
					}
					for (auto r : camerasToDestroy)
						cameras.erase(r);
				}

				{ // blit to the window
					const auto graphicsDebugScope = renderQueue->scopedNamedPass("blit to the window");
					renderQueue->resetFrameBuffer();
					viewportAndScissor(0, 0, windowWidth, windowHeight);
					setTwoSided(false);
					setDepthTest(false, false);
					const sint32 visualizeCount = numeric_cast<sint32>(visualizableTextures.size());
					const sint32 visualizeIndex = (visualizeBuffer % visualizeCount + visualizeCount) % visualizeCount;
					const VisualizableTexture &v = visualizableTextures[visualizeIndex];
					renderQueue->bind(modelSquare);
					renderQueue->bind(Holder<Texture>(v.tex, nullptr));
					if (visualizeIndex == 0)
					{
						CAGE_ASSERT(v.visualizableTextureMode == VisualizableTextureModeEnum::Color);
						renderQueue->bind(shaderVisualizeColor);
						renderQueue->uniform(0, vec2(1.0 / lastGBufferWidth, 1.0 / lastGBufferHeight));
						renderDispatch(modelSquare, 1);
					}
					else
					{
						const vec2 scale = vec2(1.0 / windowWidth, 1.0 / windowHeight);
						switch (v.visualizableTextureMode)
						{
						case VisualizableTextureModeEnum::Color:
							renderQueue->bind(shaderVisualizeColor);
							renderQueue->uniform(0, scale);
							renderDispatch(modelSquare, 1);
							break;
						case VisualizableTextureModeEnum::Depth2d:
						case VisualizableTextureModeEnum::DepthCube:
						{
							renderQueue->bind(shaderVisualizeDepth);
							renderQueue->uniform(0, scale);
							Holder<DepthMapVisualizationData> data = systemArena().createHolder<DepthMapVisualizationData>();
							data->modelSquare = modelSquare.share();
							data->tex = v.tex;
							renderQueue->customCommand(Delegate<void(void *)>().bind<&GraphicsDispatchImpl::depthMapVisualizationFunction>(), std::move(data).cast<void>(), true);
						} break;
						case VisualizableTextureModeEnum::Monochromatic:
							renderQueue->bind(shaderVisualizeMonochromatic);
							renderQueue->uniform(0, scale);
							renderDispatch(modelSquare, 1);
							break;
						case VisualizableTextureModeEnum::Velocity:
							renderQueue->bind(shaderVisualizeVelocity);
							renderQueue->uniform(0, scale);
							renderDispatch(modelSquare, 1);
							break;
						}
					}
					renderQueue->checkGlErrorDebug();
				}

				{
					GraphicsDebugScope graphicsDebugScope("dispatch render queue");
					renderQueue->dispatch();
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
				drawCalls = renderQueue->drawsCount();
				drawPrimitives = renderQueue->primitivesCount();
			}

			void swap()
			{
				CAGE_CHECK_GL_ERROR_DEBUG();
				engineWindow()->swapBuffers();
				glFinish(); // this is where the engine should be waiting for the gpu
			}
		};
	}

	GraphicsDispatch *graphicsDispatch;

	void graphicsDispatchCreate(const EngineCreateConfig &config)
	{
		graphicsDispatch = systemArena().createObject<GraphicsDispatchImpl>(config);
	}

	void graphicsDispatchDestroy()
	{
		systemArena().destroy<GraphicsDispatchImpl>(graphicsDispatch);
		graphicsDispatch = nullptr;
	}

	void graphicsDispatchInitialize()
	{
		((GraphicsDispatchImpl*)graphicsDispatch)->initialize();
	}

	void graphicsDispatchFinalize()
	{
		((GraphicsDispatchImpl*)graphicsDispatch)->finalize();
	}

	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives)
	{
		GraphicsDispatchImpl *impl = (GraphicsDispatchImpl*)graphicsDispatch;
		impl->tick();
		drawCalls = impl->drawCalls;
		drawPrimitives = impl->drawPrimitives;
	}

	void graphicsDispatchSwap()
	{
		((GraphicsDispatchImpl*)graphicsDispatch)->swap();
	}
}
