#include <cage-core/geometry.h>

#include <cage-engine/graphicsError.h>
#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/window.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/screenSpaceEffects.h>

#include "../engine.h"
#include "graphics.h"

#include <map>

namespace cage
{
	namespace
	{
		struct GraphicsDispatchHolders
		{
			Holder<Model> modelSquare, modelSphere, modelCone;
			Holder<ShaderProgram> shaderVisualizeColor, shaderFont;
			Holder<ShaderProgram> shaderAmbient, shaderBlit, shaderDepth, shaderGBuffer, shaderLighting, shaderTranslucent;
			FrameBufferHandle gBufferTarget;
			FrameBufferHandle renderTarget;
			TextureHandle albedoTexture;
			TextureHandle specialTexture;
			TextureHandle normalTexture;
			TextureHandle colorTexture;
			TextureHandle intermediateTexture;
			TextureHandle velocityTexture;
			TextureHandle ambientOcclusionTexture;
			TextureHandle depthTexture;
			TextureHandle texSource;
			TextureHandle texTarget;
			std::vector<TextureHandle> shadowmaps2d, shadowmapsCube;
		};

		struct GraphicsDispatchImpl : public GraphicsDispatch, private GraphicsDispatchHolders
		{
		public:
			uint32 drawCalls = 0;
			uint32 drawPrimitives = 0;

		private:
			Holder<RenderQueue> renderQueue;
			Holder<ProvisionalGraphics> provisionalData;
			ScreenSpaceCommonConfig gfCommonConfig; // helper to simplify initialization
			uint32 frameIndex = 0;

			void bindGBufferTextures()
			{
				const auto graphicsDebugScope = renderQueue->namedScope("bind gBuffer textures");
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
					TextureHandle &s = shadowmap > 0 ? shadowmaps2d[shadowmap - 1] : shadowmapsCube[-shadowmap - 1];
					renderQueue->bind(s, shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE);
				}
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
				renderDispatch(obj->model, numeric_cast<uint32>(obj->uniModels.size()));
			}

			void renderDispatch(const Holder<Model> &model, uint32 count)
			{
				renderQueue->draw(count);
			}

			void renderObject(const Objects *obj)
			{
				renderObject(obj, obj->model->flags);
			}

			void renderObject(const Objects *obj, ModelRenderFlags flags)
			{
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, obj->shaderConfig.shaderRoutines);
				renderQueue->universalUniformArray<Objects::UniModel>(obj->uniModels, CAGE_SHADER_UNIBLOCK_MESHES);
				if (!obj->uniArmatures.empty())
				{
					renderQueue->universalUniformArray<Mat3x4>(obj->uniArmatures, CAGE_SHADER_UNIBLOCK_ARMATURES);
					renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, obj->model->skeletonBones);
				}
				renderQueue->bind(obj->model);
				renderQueue->culling(!any(flags & ModelRenderFlags::TwoSided));
				renderQueue->depthTest(any(flags & ModelRenderFlags::DepthTest));
				renderQueue->depthWrite(any(flags & ModelRenderFlags::DepthWrite));
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
				const auto graphicsDebugScope = renderQueue->namedScope("opaque");
				OPTICK_EVENT("opaque");
				const Holder<ShaderProgram> &shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				renderQueue->bind(shr);
				for (const Holder<Objects> &o : pass->opaques)
					renderObject(+o);
				renderQueue->checkGlErrorDebug();
			}

			void renderLighting(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("lighting");
				OPTICK_EVENT("lighting");
				const Holder<ShaderProgram> &shr = shaderLighting;
				renderQueue->bind(shr);
				for (const Holder<Lights> &l : pass->lights)
				{
					renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, l->shaderConfig.shaderRoutines);
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
					renderQueue->universalUniformArray<Lights::UniLight>(l->uniLights, CAGE_SHADER_UNIBLOCK_LIGHTS);
					renderQueue->bind(model);
					renderDispatch(model, numeric_cast<uint32>(l->uniLights.size()));
				}
				renderQueue->cullFace(false);
				renderQueue->checkGlErrorDebug();
			}

			void renderTranslucent(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("translucent");
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
						renderObject(&t->object);
						std::swap(tmp, orig);
					}

					if (!t->lights.empty())
					{ // render lights on the object
						renderQueue->blendFuncAdditive();
						for (const Holder<Lights> &l : t->lights)
						{
							renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, l->shaderConfig.shaderRoutines);
							bindShadowmap(l->shadowmap);
							renderQueue->universalUniformArray<Lights::UniLight>(l->uniLights, CAGE_SHADER_UNIBLOCK_LIGHTS);
							renderDispatch(t->object.model, numeric_cast<uint32>(l->uniLights.size()));
						}
					}
				}
				renderQueue->checkGlErrorDebug();
			}

			void renderTexts(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("texts");
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
				const auto graphicsDebugScope = renderQueue->namedScope("camera pass");
				OPTICK_EVENT("camera pass");

				CAGE_ASSERT(pass->entityId != 0);
				renderCameraOpaque(pass);
				renderCameraEffectsOpaque(pass);
				renderCameraTransparencies(pass);
				renderCameraEffectsFinal(pass);

				// blit to final output texture
				CAGE_ASSERT(pass->targetTexture);
				if (texSource != pass->targetTexture)
				{
					texTarget = pass->targetTexture, nullptr;
					renderQueue->bind(shaderBlit);
					renderEffect();
				}
			}

			void renderCameraOpaque(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("deferred");
				OPTICK_EVENT("deferred");

				// opaque
				renderQueue->bind(gBufferTarget);
				renderQueue->activeAttachments(31);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();
				if (pass->clearFlags)
					renderQueue->clear(pass->clearFlags & GL_COLOR_BUFFER_BIT, pass->clearFlags & GL_DEPTH_BUFFER_BIT, pass->clearFlags & GL_STENCIL_BUFFER_BIT);
				renderOpaque(pass);
				renderQueue->culling(true);
				renderQueue->depthTest(false);
				renderQueue->depthWrite(false);
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
				renderQueue->depthTest(false);
				renderQueue->depthWrite(false);
				renderQueue->blending(false);
				renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				renderQueue->checkGlErrorDebug();
			}

			void renderCameraEffectsOpaque(const RenderPass *pass)
			{
				const auto graphicsDebugScope2 = renderQueue->namedScope("effects opaque");
				OPTICK_EVENT("effects opaque");

				renderQueue->bind(renderTarget);
				renderQueue->depthTexture(Holder<Texture>());
				renderQueue->colorTexture(0, Holder<Texture>());
				renderQueue->activeAttachments(1);

				texSource = colorTexture;
				texTarget = intermediateTexture;
				gfCommonConfig.resolution = ivec2(pass->vpW, pass->vpH);

				// ssao
				if (any(pass->effects & CameraEffectsFlags::AmbientOcclusion))
				{
					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = pass->ssao;
					cfg.frameIndex = frameIndex;
					cfg.inDepth = depthTexture;
					cfg.inNormal = normalTexture;
					cfg.outAo = ambientOcclusionTexture;
					cfg.viewProj = pass->viewProj;
					screenSpaceAmbientOcclusion(cfg);
				}

				// ambient light
				if ((pass->uniViewport.ambientLight + pass->uniViewport.ambientDirectionalLight) != vec4())
				{
					const auto graphicsDebugScope = renderQueue->namedScope("ambient light");
					renderQueue->viewport(ivec2(pass->vpX, pass->vpY), ivec2(pass->vpW, pass->vpH));
					renderQueue->bind(shaderAmbient);
					renderQueue->bind(modelSquare);
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
					ScreenSpaceDepthOfFieldConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceDepthOfField &)cfg = pass->depthOfField;
					cfg.proj = pass->proj;
					cfg.inDepth = depthTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceDepthOfField(cfg);
					std::swap(texSource, texTarget);
				}

				// motion blur
				if (any(pass->effects & CameraEffectsFlags::MotionBlur))
				{
					ScreenSpaceMotionBlurConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceMotionBlur &)cfg = pass->motionBlur;
					cfg.inVelocity = velocityTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceMotionBlur(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = pass->eyeAdaptation;
					cfg.cameraId = stringizer() + pass->entityId;
					cfg.inColor = texSource;
					screenSpaceEyeAdaptationPrepare(cfg);
				}

				renderQueue->viewport(ivec2(pass->vpX, pass->vpY), ivec2(pass->vpW, pass->vpH));
				renderQueue->bind(renderTarget);
				renderQueue->bind(texSource, CAGE_SHADER_TEXTURE_COLOR);
			}

			void renderCameraTransparencies(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("transparencies");
				OPTICK_EVENT("transparencies");

				if (pass->translucents.empty() && pass->texts.empty())
					return;

				if (texSource != colorTexture)
				{
					renderQueue->bind(shaderBlit);
					renderEffect();
				}
				CAGE_ASSERT(texSource == colorTexture);
				CAGE_ASSERT(texTarget == intermediateTexture);

				renderQueue->colorTexture(0, colorTexture);
				renderQueue->depthTexture(depthTexture);
				renderQueue->activeAttachments(1);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();

				renderQueue->depthTest(true);
				renderQueue->depthWrite(true);
				renderQueue->blending(true);
				renderQueue->checkGlErrorDebug();

				renderTranslucent(pass);

				renderQueue->depthTest(true);
				renderQueue->depthWrite(false);
				renderQueue->culling(false);
				renderQueue->blendFuncAlphaTransparency();
				renderQueue->checkGlErrorDebug();

				renderTexts(pass);

				renderQueue->depthTest(false);
				renderQueue->depthWrite(false);
				renderQueue->culling(true);
				renderQueue->blending(false);
				renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
				renderQueue->checkGlErrorDebug();

				renderQueue->depthTexture(Holder<Texture>());
				renderQueue->activeAttachments(1);
				renderQueue->bind(modelSquare);
				bindGBufferTextures();
			}

			void renderCameraEffectsFinal(const RenderPass *pass)
			{
				const auto graphicsDebugScope2 = renderQueue->namedScope("effects final");
				OPTICK_EVENT("effects final");

				gfCommonConfig.resolution = ivec2(pass->vpW, pass->vpH);

				// bloom
				if (any(pass->effects & CameraEffectsFlags::Bloom))
				{
					ScreenSpaceBloomConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceBloom &)cfg = pass->bloom;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceBloom(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = pass->eyeAdaptation;
					cfg.cameraId = stringizer() + pass->entityId;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceEyeAdaptationApply(cfg);
					std::swap(texSource, texTarget);
				}

				// final screen effects
				if (any(pass->effects & (CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					ScreenSpaceTonemapConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					(ScreenSpaceTonemap &)cfg = pass->tonemap;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					cfg.tonemapEnabled = any(pass->effects & CameraEffectsFlags::ToneMapping);
					cfg.gamma = any(pass->effects & CameraEffectsFlags::GammaCorrection) ? pass->gamma : 1;
					screenSpaceTonemap(cfg);
					std::swap(texSource, texTarget);
				}

				// fxaa
				if (any(pass->effects & CameraEffectsFlags::AntiAliasing))
				{
					ScreenSpaceFastApproximateAntiAliasingConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceFastApproximateAntiAliasing(cfg);
					std::swap(texSource, texTarget);
				}

				renderQueue->viewport(ivec2(pass->vpX, pass->vpY), ivec2(pass->vpW, pass->vpH));
				renderQueue->bind(renderTarget);
				renderQueue->bind(texSource, CAGE_SHADER_TEXTURE_COLOR);
			}

			void renderShadowPass(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("shadow pass");
				OPTICK_EVENT("shadow pass");

				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1] : shadowmapsCube[-pass->targetShadowmap - 1]);
				renderQueue->activeAttachments(0);
				renderQueue->checkFrameBuffer();
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->checkGlErrorDebug();

				const Holder<ShaderProgram> &shr = shaderDepth;
				renderQueue->bind(shr);
				for (const Holder<Objects> &o : pass->opaques)
					renderObject(+o, o->model->flags | ModelRenderFlags::DepthWrite);
				for (const Holder<Translucent> &o : pass->translucents)
					renderObject(&o->object, o->object.model->flags | ModelRenderFlags::DepthWrite);

				renderQueue->colorWrite(true);
				renderQueue->checkGlErrorDebug();
			}

			void renderPass(const RenderPass *pass)
			{
				renderQueue->universalUniformStruct(pass->uniViewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				renderQueue->viewport(ivec2(), ivec2(pass->vpW, pass->vpH));
				renderQueue->depthTest(true);
				renderQueue->scissors(true);
				renderQueue->blending(false);
				renderQueue->culling(true);
				renderQueue->depthTest(true);
				renderQueue->depthWrite(true);
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();

				if (pass->targetShadowmap == 0)
					renderCameraPass(pass);
				else
					renderShadowPass(pass);

				renderQueue->culling(true);
				renderQueue->depthTest(false);
				renderQueue->depthWrite(false);
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();
			}

			void blitToWindow(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("blit to window");
				renderQueue->resetFrameBuffer();
				renderQueue->viewport(ivec2(pass->vpX, pass->vpY), ivec2(pass->vpW, pass->vpH));
				renderQueue->culling(true);
				renderQueue->depthTest(false);
				renderQueue->depthWrite(false);
				renderQueue->bind(colorTexture, 0);
				renderQueue->bind(shaderVisualizeColor);
				renderQueue->uniform(0, vec2(1.0 / windowWidth, 1.0 / windowHeight));
				renderQueue->bind(modelSquare);
				renderQueue->draw();
				renderQueue->checkGlErrorDebug();
			}

			TextureHandle prepareTexture(const RenderPass *pass, const char *name, bool enabled, uint32 internalFormat, uint32 downscale = 1, bool mipmaps = false)
			{
				if (!enabled)
					return {};
				RenderQueue *q = +renderQueue;
				TextureHandle t = provisionalData->texture(name);
				q->bind(t, 0);
				q->filters(mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR, GL_LINEAR, 0);
				q->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				q->image2d(max(ivec2(pass->vpW, pass->vpH) / downscale, 1u), internalFormat);
				if (mipmaps)
					q->generateMipmaps();
				return t;
			}

			void prepareGBuffer(const RenderPass *pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("prepare gBuffer");

				albedoTexture = prepareTexture(pass, "albedoTexture", true, GL_RGB8);
				specialTexture = prepareTexture(pass, "specialTexture", true, GL_RG8);
				normalTexture = prepareTexture(pass, "normalTexture", true, GL_RGB16F);
				colorTexture = prepareTexture(pass, "colorTexture", true, GL_RGB16F);
				velocityTexture = prepareTexture(pass, "velocityTexture", any(pass->effects & CameraEffectsFlags::MotionBlur), GL_RG16F);
				depthTexture = prepareTexture(pass, "depthTexture", true, GL_DEPTH_COMPONENT32);
				intermediateTexture = prepareTexture(pass, "intermediateTexture", true, GL_RGB16F);
				ambientOcclusionTexture = prepareTexture(pass, "ambientOcclusionTexture", any(pass->effects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				CAGE_CHECK_GL_ERROR_DEBUG();

				RenderQueue *q = +renderQueue;
				gBufferTarget = provisionalData->frameBufferDraw("gBuffer");
				q->bind(gBufferTarget);
				q->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture);
				q->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture);
				q->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture);
				q->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture);
				q->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, any(pass->effects & CameraEffectsFlags::MotionBlur) ? velocityTexture : TextureHandle());
				q->depthTexture(depthTexture);
				q->resetFrameBuffer();
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			TextureHandle prepareShadowmap(const RenderPass *pass)
			{
				CAGE_ASSERT(pass->targetShadowmap != 0);
				TextureHandle t;
				if (pass->targetShadowmap < 0)
				{
					t = provisionalData->textureCube(stringizer() + "shadowmap_cube_" + pass->entityId);
					renderQueue->bind(t);
					renderQueue->imageCube(ivec2(pass->shadowmapResolution), GL_DEPTH_COMPONENT16);
				}
				else
				{
					t = provisionalData->texture(stringizer() + "shadowmap_2d_" + pass->entityId);
					renderQueue->bind(t);
					renderQueue->image2d(ivec2(pass->shadowmapResolution), GL_DEPTH_COMPONENT24);
				}
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 16);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->checkGlErrorDebug();
				return t;
			}

		public:
			void initialize()
			{
				renderQueue = newRenderQueue();
				provisionalData = newProvisionalGraphics();
			}

			void finalize()
			{
				*(GraphicsDispatchHolders *)this = GraphicsDispatchHolders();
				renderQueue.clear();
				provisionalData.clear();
			}

			void tick()
			{
				GraphicsDebugScope graphicsDebugScope("engine tick");

				drawCalls = 0;
				drawPrimitives = 0;
				*(GraphicsDispatchHolders *)this = GraphicsDispatchHolders();
				renderQueue->resetQueue();
				provisionalData->reset();

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				CAGE_CHECK_GL_ERROR_DEBUG();

				if (windowWidth == 0 || windowHeight == 0 || renderPasses.empty())
					return;

				AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")))
					return;

				modelSquare = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelSphere = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/sphere.obj"));
				modelCone = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/cone.obj"));
				shaderVisualizeColor = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/color.glsl"));
				shaderAmbient = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/ambient.glsl"));
				shaderBlit = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
				shaderDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/depth.glsl"));
				shaderGBuffer = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/gBuffer.glsl"));
				shaderLighting = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/lighting.glsl"));
				shaderTranslucent = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/translucent.glsl"));
				shaderFont = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
				CAGE_ASSERT(shaderBlit);

				gfCommonConfig.assets = engineAssets();
				gfCommonConfig.provisionals = +provisionalData;
				gfCommonConfig.queue = +renderQueue;
				renderTarget = provisionalData->frameBufferDraw("renderTarget");

				// render all passes
				for (const Holder<RenderPass> &pass : renderPasses)
				{
					if (pass->targetShadowmap > 0)
					{ // 2d shadowmap
						const uint32 idx = pass->targetShadowmap - 1;
						shadowmaps2d.resize(max(numeric_cast<uint32>(shadowmaps2d.size()), idx + 1));
						TextureHandle &s = shadowmaps2d[idx];
						s = prepareShadowmap(+pass);
						renderPass(+pass);
						continue;
					}
					else if (pass->targetShadowmap < 0)
					{ // cube shadowmap
						const uint32 idx = -pass->targetShadowmap - 1;
						shadowmapsCube.resize(max(numeric_cast<uint32>(shadowmapsCube.size()), idx + 1));
						TextureHandle &s = shadowmapsCube[idx];
						s = prepareShadowmap(+pass);
						renderPass(+pass);
						continue;
					}
					else
					{ // regular scene render
						CAGE_ASSERT(pass->targetShadowmap == 0);
						prepareGBuffer(+pass);
						if (pass->targetTexture)
						{ // render to texture
							renderPass(+pass);
						}
						else
						{ // render to screen
							pass->targetTexture = colorTexture;
							renderPass(+pass);
							blitToWindow(+pass);
						}
					}
				}

				{
					OPTICK_EVENT("dispatch render queue");
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
		graphicsDispatch = systemArena().createObject<GraphicsDispatchImpl>();
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
