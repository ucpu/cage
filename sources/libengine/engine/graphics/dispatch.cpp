#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/serialization.h>

#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/window.h>

#include "../engine.h"
#include "graphics.h"
#include "ssaoPoints.h"

#include <map>
#include <set>

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
				if (texture->getTarget() == GL_TEXTURE_CUBE_MAP)
					texture->imageCube(w, h, GL_DEPTH_COMPONENT16);
				else
					texture->image2d(w, h, GL_DEPTH_COMPONENT24);
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

		struct SsaoShader
		{
			mat4 viewProj;
			mat4 viewProjInv;
			vec4 params; // strength, bias, power, radius
			ivec4 iparams; // sampleCount, frameIndex
		};

		struct DofShader
		{
			mat4 projInv;
			vec4 dofNear; // near, far, blur
			vec4 dofFar; // near, far, blur
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
			uint32 width = 0;
			uint32 height = 0;
			CameraEffectsFlags cameraEffects = CameraEffectsFlags::None;
		};

		struct UboCache
		{
			// double buffered ring buffer of uniform buffers :D
			std::vector<Holder<UniformBuffer>> data;
			uint32 current = 0, last = 0, prev = 0;

			UboCache()
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

		struct GraphicsDispatchHolders
		{
			Holder<Mesh> meshSquare, meshSphere, meshCone;
			Holder<ShaderProgram> shaderVisualizeColor, shaderVisualizeDepth, shaderVisualizeMonochromatic, shaderVisualizeVelocity;
			Holder<ShaderProgram> shaderAmbient, shaderBlit, shaderDepth, shaderGBuffer, shaderLighting, shaderTranslucent;
			Holder<ShaderProgram> shaderGaussianBlur, shaderSsaoGenerate, shaderSsaoApply, shaderDofCollect, shaderDofApply, shaderMotionBlur, shaderBloomGenerate, shaderBloomApply, shaderLuminanceCollection, shaderLuminanceCopy, shaderFinalScreen, shaderFxaa;
			Holder<ShaderProgram> shaderFont;

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
			UboCache uboCacheLarge;
			UboCache uboCacheSmall;

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

			bool lastTwoSided = false;
			bool lastDepthTest = false;
			bool lastDepthWrite = false;
			
			Texture *texSource = nullptr;
			Texture *texTarget = nullptr;

			static void applyShaderRoutines(const ShaderConfig *c, const Holder<ShaderProgram> &s)
			{
				s->uniform(CAGE_SHADER_UNI_ROUTINES, c->shaderRoutines);
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
				GraphicsDebugScope graphicsDebugScope("reset all textures");
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

			void useDisposableUbo(uint32 bindIndex, PointerRange<const char> data)
			{
				UniformBuffer *ubo = data.size() > 256 ? uboCacheLarge.get() : uboCacheSmall.get();
				ubo->bind();
				if (ubo->getSize() < data.size())
					ubo->writeWhole(data, GL_DYNAMIC_DRAW);
				else
					ubo->writeRange(data, 0);
				ubo->bind(bindIndex);
			}

			template<class T>
			void useDisposableUbo(uint32 bindIndex, const T &data)
			{
				PointerRange<const T> r = { &data, &data + 1 };
				useDisposableUbo(bindIndex, bufferCast<const char>(r));
			}

			template<class T>
			void useDisposableUbo(uint32 bindIndex, const std::vector<T> &data)
			{
				useDisposableUbo(bindIndex, bufferCast<const char, const T>(data));
			}

			void bindGBufferTextures()
			{
				GraphicsDebugScope graphicsDebugScope("bind gBuffer textures");
				const uint32 tius[] = { CAGE_SHADER_TEXTURE_ALBEDO, CAGE_SHADER_TEXTURE_SPECIAL, CAGE_SHADER_TEXTURE_NORMAL, CAGE_SHADER_TEXTURE_COLOR, CAGE_SHADER_TEXTURE_DEPTH };
				const Texture *texs[] = { albedoTexture.get(), specialTexture.get(), normalTexture.get(), colorTexture.get(), depthTexture.get() };
				Texture::multiBind(tius, texs);
			}

			void bindShadowmap(sint32 shadowmap)
			{
				if (shadowmap != 0)
				{
					activeTexture((shadowmap > 0 ? CAGE_SHADER_TEXTURE_SHADOW : CAGE_SHADER_TEXTURE_SHADOW_CUBE));
					ShadowmapBuffer &s = shadowmap > 0 ? shadowmaps2d[shadowmap - 1] : shadowmapsCube[-shadowmap - 1];
					s.texture->bind();
				}
			}

			void gaussianBlur(const Holder<Texture> &texData, const Holder<Texture> &texHelper, uint32 mipmapLevel = 0)
			{
				shaderGaussianBlur->bind();
				shaderGaussianBlur->uniform(1, (int)mipmapLevel);
				auto blur = [&](const Holder<Texture> &tex1, const Holder<Texture> &tex2, const vec2 &direction)
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
				CAGE_CHECK_GL_ERROR_DEBUG();
				renderTarget->colorTexture(0, texTarget);
#ifdef CAGE_DEBUG
				renderTarget->checkStatus();
#endif // CAGE_DEBUG
				texSource->bind();
				renderDispatch(meshSquare, 1);
				std::swap(texSource, texTarget);
			}

			void renderDispatch(const Objects *obj)
			{
				renderDispatch(obj->mesh, numeric_cast<uint32>(obj->uniMeshes.size()));
			}

			void renderDispatch(const Holder<Mesh> &mesh, uint32 count)
			{
				mesh->dispatch(count);
				drawCalls++;
				drawPrimitives += count * mesh->getPrimitivesCount();
			}

			void RenderObject(const Objects *obj, const Holder<ShaderProgram> &shr)
			{
				RenderObject(obj, shr, obj->mesh->getFlags());
			}

			void RenderObject(const Objects *obj, const Holder<ShaderProgram> &shr, MeshRenderFlags flags)
			{
				applyShaderRoutines(&obj->shaderConfig, shr);
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_MESHES, obj->uniMeshes);
				if (!obj->uniArmatures.empty())
				{
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_ARMATURES, obj->uniArmatures);
					shr->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, obj->mesh->getSkeletonBones());
				}
				obj->mesh->bind();
				setTwoSided(any(flags & MeshRenderFlags::TwoSided));
				setDepthTest(any(flags & MeshRenderFlags::DepthTest), any(flags & MeshRenderFlags::DepthWrite));
				{ // bind textures
					uint32 tius[MaxTexturesCountPerMaterial];
					const Texture *texs[MaxTexturesCountPerMaterial];
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
							texs[i] = obj->textures[i].get();
						}
						else
						{
							tius[i] = 0;
							texs[i] = nullptr;
						}
					}
					Texture::multiBind(tius, texs);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
				renderDispatch(obj);
			}

			void renderOpaque(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("opaque");
				OPTICK_EVENT("opaque");
				const Holder<ShaderProgram> &shr = pass->targetShadowmap ? shaderDepth : shaderGBuffer;
				shr->bind();
				for (const Holder<Objects> &o : pass->opaques)
					RenderObject(o.get(), shr);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderLighting(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("lighting");
				OPTICK_EVENT("lighting");
				const Holder<ShaderProgram> &shr = shaderLighting;
				shr->bind();
				for (const Holder<Lights> &l : pass->lights)
				{
					applyShaderRoutines(&l->shaderConfig, shr);
					Holder<Mesh> mesh;
					switch (l->lightType)
					{
					case LightTypeEnum::Directional:
						mesh = meshSquare.share();
						glCullFace(GL_BACK);
						break;
					case LightTypeEnum::Spot:
						mesh = meshCone.share();
						glCullFace(GL_FRONT);
						break;
					case LightTypeEnum::Point:
						mesh = meshSphere.share();
						glCullFace(GL_FRONT);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
					bindShadowmap(l->shadowmap);
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->uniLights);
					mesh->bind();
					renderDispatch(mesh, numeric_cast<uint32>(l->uniLights.size()));
				}
				glCullFace(GL_BACK);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTranslucent(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("translucent");
				OPTICK_EVENT("translucent");
				const Holder<ShaderProgram> &shr = shaderTranslucent;
				shr->bind();
				for (const Holder<Translucent> &t : pass->translucents)
				{
					{ // render ambient object
						glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // assume premultiplied alpha
						uint32 tmp = CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE;
						uint32 &orig = const_cast<uint32&>(t->object.shaderConfig.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE]);
						std::swap(tmp, orig);
						RenderObject(&t->object, shr);
						std::swap(tmp, orig);
					}

					if (!t->lights.empty())
					{ // render lights on the object
						glBlendFunc(GL_ONE, GL_ONE); // assume premultiplied alpha
						for (const Holder<Lights> &l : t->lights)
						{
							applyShaderRoutines(&l->shaderConfig, shr);
							bindShadowmap(l->shadowmap);
							useDisposableUbo(CAGE_SHADER_UNIBLOCK_LIGHTS, l->uniLights);
							renderDispatch(t->object.mesh, numeric_cast<uint32>(l->uniLights.size()));
						}
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderTexts(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("texts");
				OPTICK_EVENT("texts");
				for (const Holder<Texts> &t : pass->texts)
				{
					t->font->bind(meshSquare.get(), shaderFont.get());
					for (const Holder<Texts::Render> &r : t->renders)
					{
						shaderFont->uniform(0, r->transform);
						shaderFont->uniform(4, r->color);
						t->font->render(r->glyphs, r->format);
						drawCalls += numeric_cast<uint32>(r->glyphs.size() + CAGE_SHADER_MAX_CHARACTERS - 1) / CAGE_SHADER_MAX_CHARACTERS;
						drawPrimitives += numeric_cast<uint32>(r->glyphs.size()) * 2;
					}
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderCameraPass(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("camera pass");
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
				if (texSource != pass->targetTexture)
				{
					texTarget = pass->targetTexture;
					shaderBlit->bind();
					renderEffect();
				}
			}

			void renderCameraOpaque(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("deferred");
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

			void renderCameraEffectsOpaque(const RenderPass *pass, CameraSpecificData &cs)
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
					GraphicsDebugScope graphicsDebugScope("ssao");
					viewportAndScissor(pass->vpX / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpY / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpW / CAGE_SHADER_SSAO_DOWNSCALE, pass->vpH / CAGE_SHADER_SSAO_DOWNSCALE);
					{
						SsaoShader s;
						s.viewProj = pass->viewProj;
						s.viewProjInv = pass->uniViewport.vpInv;
						s.params = vec4(pass->ssao.strength, pass->ssao.bias, pass->ssao.power, pass->ssao.worldRadius);
						s.iparams[0] = pass->ssao.samplesCount;
						s.iparams[1] = frameIndex;
						useDisposableUbo(CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES, s);
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
				if ((pass->uniViewport.ambientLight + pass->uniViewport.ambientDirectionalLight) != vec4())
				{
					GraphicsDebugScope graphicsDebugScope("ambient light");
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

				// depth of field
				if (any(pass->effects & CameraEffectsFlags::DepthOfField))
				{
					GraphicsDebugScope graphicsDebugScope("depth of field");
					{
						const real fd = pass->depthOfField.focusDistance;
						const real fr = pass->depthOfField.focusRadius;
						const real br = pass->depthOfField.blendRadius;
						const real bs = pass->depthOfField.blurStrength;
						DofShader s;
						s.projInv = inverse(pass->proj);
						s.dofNear = vec4(fd - fr - br, fd - fr, bs, 0);
						s.dofFar = vec4(fd + fr, fd + fr + br, bs, 0);
						useDisposableUbo(CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES, s);
					}
					texSource->bind();
					shaderDofCollect->bind();
					viewportAndScissor(pass->vpX / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpY / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpW / CAGE_SHADER_BLOOM_DOWNSCALE, pass->vpH / CAGE_SHADER_BLOOM_DOWNSCALE);
					{ // collect near
						renderTarget->colorTexture(0, bloomTexture1.get());
						renderTarget->checkStatus();
						shaderDofCollect->uniform(0, 0);
						renderDispatch(meshSquare, 1);
					}
					{ // collect far
						renderTarget->colorTexture(0, bloomTexture2.get());
						renderTarget->checkStatus();
						shaderDofCollect->uniform(0, 1);
						renderDispatch(meshSquare, 1);
					}
					viewportAndScissor(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
					{ // apply
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS + 0);
						bloomTexture1->bind();
						bloomTexture1->generateMipmaps();
						activeTexture(CAGE_SHADER_TEXTURE_EFFECTS + 1);
						bloomTexture2->bind();
						bloomTexture2->generateMipmaps();
						activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						shaderDofApply->bind();
						renderEffect();
					}
				}

				// motion blur
				if (any(pass->effects & CameraEffectsFlags::MotionBlur))
				{
					GraphicsDebugScope graphicsDebugScope("motion blur");
					activeTexture(CAGE_SHADER_TEXTURE_EFFECTS);
					velocityTexture->bind();
					activeTexture(CAGE_SHADER_TEXTURE_COLOR);
					shaderMotionBlur->bind();
					renderEffect();
				}

				// eye adaptation
				if (any(pass->effects & CameraEffectsFlags::EyeAdaptation))
				{
					GraphicsDebugScope graphicsDebugScope("eye adaptation");
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

			void renderCameraTransparencies(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("transparencies");
				OPTICK_EVENT("transparencies");

				if (pass->translucents.empty() && pass->texts.empty())
					return;

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

			void renderCameraEffectsFinal(const RenderPass *pass, CameraSpecificData &cs)
			{
				GraphicsDebugScope graphicsDebugScope("effects final");
				OPTICK_EVENT("effects final");

				// bloom
				if (any(pass->effects & CameraEffectsFlags::Bloom))
				{
					GraphicsDebugScope graphicsDebugScope("bloom");
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
						shaderBloomApply->bind();
						shaderBloomApply->uniform(0, (int)pass->bloom.blurPasses);
						renderEffect();
					}
				}

				// final screen effects
				if (any(pass->effects & (CameraEffectsFlags::EyeAdaptation | CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					GraphicsDebugScope graphicsDebugScope("final screen");
					FinalScreenShader f;
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
					useDisposableUbo(CAGE_SHADER_UNIBLOCK_EFFECT_PROPERTIES, f);
					shaderFinalScreen->bind();
					renderEffect();
				}

				// fxaa
				if (any(pass->effects & CameraEffectsFlags::AntiAliasing))
				{
					GraphicsDebugScope graphicsDebugScope("fxaa");
					shaderFxaa->bind();
					renderEffect();
				}
			}

			void renderShadowPass(const RenderPass *pass)
			{
				GraphicsDebugScope graphicsDebugScope("shadow pass");
				OPTICK_EVENT("shadow pass");

				renderTarget->bind();
				renderTarget->clear();
				renderTarget->depthTexture(pass->targetShadowmap > 0 ? shadowmaps2d[pass->targetShadowmap - 1].texture.get() : shadowmapsCube[-pass->targetShadowmap - 1].texture.get());
				renderTarget->activeAttachments(0);
				renderTarget->checkStatus();
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glClear(GL_DEPTH_BUFFER_BIT);
				CAGE_CHECK_GL_ERROR_DEBUG();

				const Holder<ShaderProgram> &shr = shaderDepth;
				shr->bind();
				for (const Holder<Objects> &o : pass->opaques)
					RenderObject(o.get(), shr, o->mesh->getFlags() | MeshRenderFlags::DepthWrite);
				for (const Holder<Translucent> &o : pass->translucents)
					RenderObject(&o->object, shr, o->object.mesh->getFlags() | MeshRenderFlags::DepthWrite);

				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				CAGE_CHECK_GL_ERROR_DEBUG();
			}

			void renderPass(const RenderPass *pass)
			{
				useDisposableUbo(CAGE_SHADER_UNIBLOCK_VIEWPORT, pass->uniViewport);
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
					texture->image2d(max(lastGBufferWidth / downscale, 1u), max(lastGBufferHeight / downscale, 1u), internalFormat);
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
				resizeTexture("ambientOcclusionTexture1", ambientOcclusionTexture1, any(cameraEffects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				resizeTexture("ambientOcclusionTexture2", ambientOcclusionTexture2, any(cameraEffects & CameraEffectsFlags::AmbientOcclusion), GL_R8, CAGE_SHADER_SSAO_DOWNSCALE);
				resizeTexture("bloomTexture1", bloomTexture1, any(cameraEffects & (CameraEffectsFlags::Bloom | CameraEffectsFlags::DepthOfField)), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE, true);
				resizeTexture("bloomTexture2", bloomTexture2, any(cameraEffects & (CameraEffectsFlags::Bloom | CameraEffectsFlags::DepthOfField)), GL_RGB16F, CAGE_SHADER_BLOOM_DOWNSCALE, true);
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

				ssaoPointsBuffer = newUniformBuffer();
				ssaoPointsBuffer->setDebugName("ssaoPointsBuffer");
				{
					const vec4 *points = nullptr;
					uint32 count = 0;
					pointsForSsaoShader(points, count);
					PointerRange<const vec4> r = { points, points + count };
					ssaoPointsBuffer->writeWhole(bufferCast<const char>(r), GL_STATIC_DRAW);
				}
				CAGE_CHECK_GL_ERROR_DEBUG();
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
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return;

				meshSquare = ass->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/square.obj"));
				meshSphere = ass->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/sphere.obj"));
				meshCone = ass->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/cone.obj"));
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
				shaderSsaoGenerate = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoGenerate.glsl"));
				shaderSsaoApply = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoApply.glsl"));
				shaderDofCollect = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/dofCollect.glsl"));
				shaderDofApply = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/dofApply.glsl"));
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
				if (ambientOcclusionTexture2)
					visualizableTextures.emplace_back(ambientOcclusionTexture2.get(), VisualizableTextureModeEnum::Monochromatic);
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

				ssaoPointsBuffer->bind(CAGE_SHADER_UNIBLOCK_SSAO_POINTS);
				uboCacheLarge.frame();
				uboCacheSmall.frame();
				CAGE_CHECK_GL_ERROR_DEBUG();

				{ // render all passes
					std::set<uintPtr> camerasToDestroy;
					for (auto &cs : cameras)
						camerasToDestroy.insert(cs.first);
					for (const Holder<RenderPass> &pass : renderPasses)
					{
						renderPass(pass.get());
						CAGE_CHECK_GL_ERROR_DEBUG();
						camerasToDestroy.erase(pass->entityId);
					}
					for (auto r : camerasToDestroy)
						cameras.erase(r);
				}

				{ // blit to the window
					GraphicsDebugScope graphicsDebugScope("blit to the window");
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					viewportAndScissor(0, 0, windowWidth, windowHeight);
					setTwoSided(false);
					setDepthTest(false, false);
					const sint32 visualizeCount = numeric_cast<sint32>(visualizableTextures.size());
					const sint32 visualizeIndex = (visualizeBuffer % visualizeCount + visualizeCount) % visualizeCount;
					VisualizableTexture &v = visualizableTextures[visualizeIndex];
					meshSquare->bind();
					v.tex->bind();
					if (visualizeIndex == 0)
					{
						CAGE_ASSERT(v.visualizableTextureMode == VisualizableTextureModeEnum::Color);
						shaderVisualizeColor->bind();
						shaderVisualizeColor->uniform(0, vec2(1.0 / lastGBufferWidth, 1.0 / lastGBufferHeight));
						renderDispatch(meshSquare, 1);
					}
					else
					{
						vec2 scale = vec2(1.0 / windowWidth, 1.0 / windowHeight);
						switch (v.visualizableTextureMode)
						{
						case VisualizableTextureModeEnum::Color:
							shaderVisualizeColor->bind();
							shaderVisualizeColor->uniform(0, scale);
							renderDispatch(meshSquare, 1);
							break;
						case VisualizableTextureModeEnum::Depth2d:
						case VisualizableTextureModeEnum::DepthCube:
						{
							shaderVisualizeDepth->bind();
							shaderVisualizeDepth->uniform(0, scale);
							GLint cmpMode = 0;
							glGetTexParameteriv(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, &cmpMode);
							glTexParameteri(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, GL_NONE);
							renderDispatch(meshSquare, 1);
							glTexParameteri(v.tex->getTarget(), GL_TEXTURE_COMPARE_MODE, cmpMode);
						} break;
						case VisualizableTextureModeEnum::Monochromatic:
							shaderVisualizeMonochromatic->bind();
							shaderVisualizeMonochromatic->uniform(0, scale);
							renderDispatch(meshSquare, 1);
							break;
						case VisualizableTextureModeEnum::Velocity:
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
				engineWindow()->swapBuffers();
				glFinish(); // this is where the engine should be waiting for the gpu
			}
		};
	}

	GraphicsDispatch *graphicsDispatch;

	void graphicsDispatchCreate(const EngineCreateConfig &config)
	{
		graphicsDispatch = detail::systemArena().createObject<GraphicsDispatchImpl>(config);
	}

	void graphicsDispatchDestroy()
	{
		detail::systemArena().destroy<GraphicsDispatchImpl>(graphicsDispatch);
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
