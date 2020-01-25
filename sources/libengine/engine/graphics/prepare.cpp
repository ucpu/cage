#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetStructs.h>
#include <cage-core/hashString.h>
#include <cage-core/color.h>
#include <cage-core/swapBufferGuard.h>

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

#include <algorithm>
#include <unordered_map>

namespace cage
{
	// implemented in gui
	string loadInternationalizedText(AssetManager *assets, uint32 asset, uint32 text, string params);

	namespace
	{
		ConfigBool confRenderMissingMeshes("cage/graphics/renderMissingMeshes", false);
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);
		ConfigBool confNoAmbientOcclusion("cage/graphics/disableAmbientOcclusion", false);
		ConfigBool confNoBloom("cage/graphics/disableBloom", false);
		ConfigBool confNoMotionBlur("cage/graphics/disableMotionBlur", false);
		ConfigBool confNoNormalMap("cage/graphics/disableNormalMaps", false);

		struct ShadowmapImpl : public ShadowmapComponent
		{
			mat4 shadowMat;
			sint32 index = 0;

			explicit ShadowmapImpl(const ShadowmapComponent &other) : ShadowmapComponent(other)
			{}
		};

		struct EmitTransforms
		{
			TransformComponent current, history;
			mat4 model;
			mat4 modelPrev;

			void updateModelMatrix(real interFactor)
			{
				model = mat4(interpolate(history, current, interFactor));
				modelPrev = mat4(interpolate(history, current, interFactor - 0.2));
			}
		};

		struct EmitRender : public EmitTransforms
		{
			RenderComponent render;
			Holder<TextureAnimationComponent> animatedTexture;
			Holder<SkeletalAnimationComponent> animatedSkeleton;
		};

		struct EmitText : public EmitTransforms
		{
			TextComponent renderText;
		};

		struct EmitLight : public EmitTransforms
		{
			LightComponent light;
			Holder<ShadowmapImpl> shadowmap;
			uintPtr entityId = 0;
		};

		struct EmitCamera : public EmitTransforms
		{
			CameraComponent camera;
			uintPtr entityId = 0;
		};

		struct RenderPassImpl : public RenderPass
		{
			mat4 viewProjPrev;
			uint32 sceneMask = 0;
			real lodSelection = 0; // vertical size of screen, at distance of one world-space-unit from camera, in pixels
			bool lodOrthographic = false;
		};

		struct Emit
		{
			std::vector<EmitRender> renderableObjects;
			std::vector<EmitText> renderableTexts;
			std::vector<EmitLight> lights;
			std::vector<EmitCamera> cameras;

			uint64 time = 0;
			bool fresh = false;

			explicit Emit(const EngineCreateConfig &config)
			{
				renderableObjects.reserve(256);
				renderableTexts.reserve(64);
				lights.reserve(32);
				cameras.reserve(4);
			}
		};

		struct GraphicsPrepareImpl
		{
			Emit emitBufferA, emitBufferB, emitBufferC; // this is awfully stupid, damn you c++
			Emit *emitBuffers[3] = { nullptr };
			Emit *emitRead = nullptr, *emitWrite = nullptr;
			Holder<SwapBufferGuard> swapController;

			mat4 tmpArmature[CAGE_SHADER_MAX_BONES];
			mat4 tmpArmature2[CAGE_SHADER_MAX_BONES];

			InterpolationTimingCorrector itc;
			uint64 emitTime = 0;
			uint64 dispatchTime = 0;
			uint64 lastDispatchTime = 0;
			uint64 elapsedDispatchTime = 0;
			sint32 shm2d = 0, shmCube = 0;

			typedef std::unordered_map<Mesh*, Objects*> OpaqueObjectsMap;
			OpaqueObjectsMap opaqueObjectsMap;
			typedef std::unordered_map<Font*, Texts*> TextsMap;
			TextsMap textsMap;

			Holder<Mesh> meshSphere, meshCone;

			static real lightRange(const vec3 &color, const vec3 &attenuation)
			{
				real c = max(color[0], max(color[1], color[2]));
				if (c <= 1e-5)
					return 0;
				real e = c * 100;
				real x = attenuation[0], y = attenuation[1], z = attenuation[2];
				if (z < 1e-5)
				{
					CAGE_ASSERT(y > 1e-5, "invalid light attenuation", attenuation);
					return (e - x) / y;
				}
				return (sqrt(y * y - 4 * z * (x - e)) - y) / (2 * z);
			}

			static mat4 lightSpotCone(real range, rads angle)
			{
				real xy = tan(angle * 0.5);
				vec3 scale = vec3(xy, xy, 1) * range;
				return mat4(vec3(), quat(), scale);
			}

			RenderPassImpl *newRenderPass()
			{
				opaqueObjectsMap.clear();
				textsMap.clear();
				Holder<RenderPassImpl> t = detail::systemArena().createHolder<RenderPassImpl>();
				RenderPassImpl *r = t.get();
				graphicsDispatch->renderPasses.push_back(templates::move(t).cast<RenderPass>());
				return r;
			}

			static real fovToLodSelection(rads fov)
			{
				return tan(fov * 0.5) * 2;
			}

			static void sortTranslucentBackToFront(RenderPassImpl *pass)
			{
				if (pass->translucents.empty())
					return;
				OPTICK_EVENT("sort translucent");

				std::vector<real> distances;
				std::vector<uint32> order;
				{
					const vec3 center = vec3(pass->uniViewport.eyePos);
					distances.reserve(pass->translucents.size());
					order.reserve(pass->translucents.size());
					uint32 idx = 0;
					for (const auto &it : pass->translucents)
					{
						const vec4 *m = it->object.uniMeshes[0].mMat.data;
						const vec3 p = vec3(m[0][3], m[1][3], m[2][3]);
						distances.push_back(distanceSquared(center, p));
						order.push_back(idx++);
					}
				}
				std::sort(order.begin(), order.end(), [&](uint32 a, uint32 b) {
					return distances[a] < distances[b];
					});
				std::vector<Holder<Translucent>> result;
				result.reserve(pass->translucents.size());
				for (uint32 o : order)
					result.push_back(templates::move(pass->translucents[o]));
				std::swap(result, pass->translucents);
			}

			static void initializeStereoCamera(RenderPassImpl *pass, EmitCamera *camera, StereoEyeEnum eye, const mat4 &model)
			{
				StereoCameraInput in;
				in.position = vec3(model * vec4(0, 0, 0, 1));
				in.orientation = quat(mat3(model));
				in.viewportOrigin = camera->camera.viewportOrigin;
				in.viewportSize = camera->camera.viewportSize;
				in.aspectRatio = real(graphicsDispatch->windowWidth) / real(graphicsDispatch->windowHeight);
				in.fov = camera->camera.camera.perspectiveFov;
				in.far = camera->camera.far;
				in.near = camera->camera.near;
				in.zeroParallaxDistance = camera->camera.zeroParallaxDistance;
				in.eyeSeparation = camera->camera.eyeSeparation;
				in.orthographic = camera->camera.cameraType == CameraTypeEnum::Orthographic;
				StereoCameraOutput out = stereoCamera(in, (StereoModeEnum)graphicsPrepareThread().stereoMode, eye);
				pass->view = out.view;
				pass->proj = out.projection;
				if (camera->camera.cameraType == CameraTypeEnum::Perspective)
					pass->lodSelection = fovToLodSelection(camera->camera.camera.perspectiveFov) * graphicsDispatch->windowHeight;
				else
				{
					const vec2 &os = camera->camera.camera.orthographicSize;
					pass->proj = orthographicProjection(-os[0], os[0], -os[1], os[1], camera->camera.near, camera->camera.far);
					pass->lodSelection = os[1] * graphicsDispatch->windowHeight;
					pass->lodOrthographic = true;
				}
				pass->viewProj = pass->proj * pass->view;
				pass->vpX = numeric_cast<uint32>(out.viewportOrigin[0] * real(graphicsDispatch->windowWidth));
				pass->vpY = numeric_cast<uint32>(out.viewportOrigin[1] * real(graphicsDispatch->windowHeight));
				pass->vpW = numeric_cast<uint32>(out.viewportSize[0] * real(graphicsDispatch->windowWidth));
				pass->vpH = numeric_cast<uint32>(out.viewportSize[1] * real(graphicsDispatch->windowHeight));
			}

			static void initializeTargetCamera(RenderPassImpl *pass, const mat4 &model)
			{
				vec3 p = vec3(model * vec4(0, 0, 0, 1));
				pass->view = lookAt(p, p + vec3(model * vec4(0, 0, -1, 0)), vec3(model * vec4(0, 1, 0, 0)));
				pass->viewProj = pass->proj * pass->view;
			}

			void initializeRenderPassForCamera(RenderPassImpl *pass, EmitCamera *camera, StereoEyeEnum eye)
			{
				OPTICK_EVENT("camera pass");
				if (camera->camera.target)
				{
					OPTICK_TAG("target", (uintPtr)camera->camera.target);
					uint32 w = 0, h = 0;
					camera->camera.target->getResolution(w, h);
					if (w == 0 || h == 0)
						CAGE_THROW_ERROR(Exception, "target texture has zero resolution");
					switch (camera->camera.cameraType)
					{
					case CameraTypeEnum::Orthographic:
					{
						const vec2 &os = camera->camera.camera.orthographicSize;
						pass->proj = orthographicProjection(-os[0], os[0], -os[1], os[1], camera->camera.near, camera->camera.far);
						pass->lodSelection = os[1] * h;
						pass->lodOrthographic = true;
					} break;
					case CameraTypeEnum::Perspective:
						pass->proj = perspectiveProjection(camera->camera.camera.perspectiveFov, real(w) / real(h), camera->camera.near, camera->camera.far);
						pass->lodSelection = fovToLodSelection(camera->camera.camera.perspectiveFov) * h;
						break;
					default:
						CAGE_THROW_ERROR(Exception, "invalid camera type");
					}
					initializeTargetCamera(pass, camera->modelPrev);
					pass->viewProjPrev = pass->viewProj;
					initializeTargetCamera(pass, camera->model);
					pass->vpX = numeric_cast<uint32>(camera->camera.viewportOrigin[0] * w);
					pass->vpY = numeric_cast<uint32>(camera->camera.viewportOrigin[1] * h);
					pass->vpW = numeric_cast<uint32>(camera->camera.viewportSize[0] * w);
					pass->vpH = numeric_cast<uint32>(camera->camera.viewportSize[1] * h);
				}
				else
				{
					OPTICK_TAG("eye", (int)eye);
					initializeStereoCamera(pass, camera, eye, camera->modelPrev);
					pass->viewProjPrev = pass->viewProj;
					initializeStereoCamera(pass, camera, eye, camera->model);
				}
				pass->uniViewport.vpInv = inverse(pass->viewProj);
				pass->uniViewport.eyePos = camera->model * vec4(0, 0, 0, 1);
				pass->uniViewport.eyeDir = camera->model * vec4(0, 0, -1, 0);
				pass->uniViewport.ambientLight = vec4(camera->camera.ambientLight, 0);
				pass->uniViewport.ambientDirectionalLight = vec4(camera->camera.ambientDirectionalLight, 0);
				pass->uniViewport.viewport = vec4(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				pass->targetTexture = camera->camera.target;
				pass->clearFlags = ((camera->camera.clear & CameraClearFlags::Color) == CameraClearFlags::Color ? GL_COLOR_BUFFER_BIT : 0) | ((camera->camera.clear & CameraClearFlags::Depth) == CameraClearFlags::Depth ? GL_DEPTH_BUFFER_BIT : 0);
				pass->sceneMask = camera->camera.sceneMask;
				pass->entityId = camera->entityId;
				(CameraEffects&)*pass = (CameraEffects&)camera->camera;
				real eyeAdaptationSpeed = real(elapsedDispatchTime) * 1e-6;
				pass->eyeAdaptation.darkerSpeed *= eyeAdaptationSpeed;
				pass->eyeAdaptation.lighterSpeed *= eyeAdaptationSpeed;
				OPTICK_TAG("x", pass->vpX);
				OPTICK_TAG("y", pass->vpY);
				OPTICK_TAG("width", pass->vpW);
				OPTICK_TAG("height", pass->vpH);
				OPTICK_TAG("sceneMask", pass->sceneMask);
				addRenderableObjects(pass);
				addRenderableLights(pass);
				addRenderableTexts(pass);
				sortTranslucentBackToFront(pass);
			}

			void initializeRenderPassForShadowmap(RenderPassImpl *pass, EmitLight *light)
			{
				OPTICK_EVENT("shadowmap pass");
				pass->view = inverse(light->model);
				switch (light->light.lightType)
				{
				case LightTypeEnum::Directional:
					pass->proj = orthographicProjection(-light->shadowmap->worldSize[0], light->shadowmap->worldSize[0], -light->shadowmap->worldSize[1], light->shadowmap->worldSize[1], -light->shadowmap->worldSize[2], light->shadowmap->worldSize[2]);
					pass->lodSelection = light->shadowmap->worldSize[1] * 2;
					pass->lodOrthographic = true;
					break;
				case LightTypeEnum::Spot:
					pass->proj = perspectiveProjection(light->light.spotAngle, 1, light->shadowmap->worldSize[0], light->shadowmap->worldSize[1]);
					pass->lodSelection = fovToLodSelection(light->light.spotAngle) * light->shadowmap->resolution;
					break;
				case LightTypeEnum::Point:
					pass->proj = perspectiveProjection(degs(90), 1, light->shadowmap->worldSize[0], light->shadowmap->worldSize[1]);
					pass->lodSelection = fovToLodSelection(degs(90)) * light->shadowmap->resolution;
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
				pass->viewProj = pass->proj * pass->view;
				pass->viewProjPrev = pass->viewProj;
				pass->shadowmapResolution = pass->vpW = pass->vpH = light->shadowmap->resolution;
				pass->sceneMask = 0xffffffff;
				pass->targetShadowmap = light->light.lightType == LightTypeEnum::Point ? (-shmCube++ - 1) : (shm2d++ + 1);
				light->shadowmap->index = pass->targetShadowmap;
				static const mat4 bias = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				light->shadowmap->shadowMat = bias * pass->viewProj;
				pass->entityId = light->entityId;
				OPTICK_TAG("resolution", pass->shadowmapResolution);
				OPTICK_TAG("sceneMask", pass->sceneMask);
				addRenderableObjects(pass);
			}

			void addRenderableObjects(RenderPassImpl *pass)
			{
				OPTICK_EVENT("objects");
				CAGE_ASSERT(pass->lodSelection > 0);
				for (EmitRender &e : emitRead->renderableObjects)
				{
					if ((e.render.sceneMask & pass->sceneMask) == 0 || e.render.object == 0)
						continue;

					{
						Holder<Mesh> m = engineAssets()->tryGet<AssetSchemeIndexMesh, Mesh>(e.render.object);
						if (m)
						{
							addRenderableMesh(pass, &e, templates::move(m));
							continue;
						}
					}

					{
						Holder<RenderObject> o = engineAssets()->get<AssetSchemeIndexRenderObject, RenderObject>(e.render.object);
						if (!o || o->lodsCount() == 0)
							continue;
						uint32 lod = 0;
						if (o->lodsCount() > 1)
						{
							real d = 1;
							if (!pass->lodOrthographic)
							{
								vec4 ep4 = e.model * vec4(0, 0, 0, 1);
								d = distance(vec3(ep4) / ep4[3], vec3(pass->uniViewport.eyePos));
							}
							real f = pass->lodSelection * o->worldSize / (d * o->pixelsSize);
							lod = o->lodSelect(f.value);
						}
						for (uint32 msh : o->meshes(lod))
						{
							Holder<Mesh> m = engineAssets()->get<AssetSchemeIndexMesh, Mesh>(msh);
							if (m)
								addRenderableMesh(pass, &e, templates::move(m));
						}
					}
				}
			}

			void addRenderableSkeleton(RenderPassImpl *pass, EmitRender *e, Holder<SkeletonRig> s, const mat4 &model, const mat4 &mvp)
			{
				uint32 bonesCount = s->bonesCount();
				bool initialized = false;
				if (e->animatedSkeleton)
				{
					const auto &ba = *e->animatedSkeleton;
					Holder<SkeletalAnimation> an = engineAssets()->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ba.name);
					if (an)
					{
						CAGE_ASSERT(s->bonesCount() == bonesCount, s->bonesCount(), bonesCount);
						real c = detail::evalCoefficientForSkeletalAnimation(an.get(), dispatchTime, ba.startTime, ba.speed, ba.offset);
						s->animateSkeleton(an.get(), c, tmpArmature, tmpArmature2);
						initialized = true;
					}
				}
				if (!initialized)
				{
					for (uint32 i = 0; i < bonesCount; i++)
						tmpArmature2[i] = mat4();
				}
				Holder<Mesh> mesh = engineAssets()->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/bone.obj"));
				if (!mesh)
					return;
				CAGE_ASSERT(mesh->getSkeletonName() == 0);
				for (uint32 i = 0; i < bonesCount; i++)
				{
					e->render.color = colorHsvToRgb(vec3(real(i) / real(bonesCount), 1, 1));
					mat4 m = model * tmpArmature2[i];
					mat4 mvp = pass->viewProj * m;
					addRenderableMesh(pass, e, mesh.share(), m, mvp, mvp);
				}
			}

			void addRenderableMesh(RenderPassImpl *pass, EmitRender *e, Holder<Mesh> m)
			{
				addRenderableMesh(pass, e, templates::move(m), e->model, pass->viewProj * e->model, pass->viewProjPrev * e->modelPrev);
			}

			void addRenderableMesh(RenderPassImpl *pass, EmitRender *e, Holder<Mesh> m, const mat4 &model, const mat4 &mvp, const mat4 &mvpPrev)
			{
				if (!frustumCulling(m->getBoundingBox(), mvp))
					return;
				if (pass->targetShadowmap != 0 && none(m->getFlags() & MeshRenderFlags::ShadowCast))
					return;
				if (m->getSkeletonName() && confRenderSkeletonBones)
				{
					Holder<SkeletonRig> s = engineAssets()->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(m->getSkeletonName());
					addRenderableSkeleton(pass, e, templates::move(s), model, mvp);
					return;
				}
				Objects *obj = nullptr;
				if (any(m->getFlags() & MeshRenderFlags::Translucency) || e->render.opacity < 1)
				{ // translucent
					pass->translucents.push_back(detail::systemArena().createHolder<Translucent>(m.share()));
					Translucent *t = pass->translucents.back().get();
					obj = &t->object;
					if (any(m->getFlags() & MeshRenderFlags::Lighting))
					{
						for (EmitLight &it : emitRead->lights)
							addLight(t, mvp, &it); // todo pass other parameters needed for the intersection tests
					}
				}
				else
				{ // opaque
					auto it = opaqueObjectsMap.find(m.get());
					if (it == opaqueObjectsMap.end())
					{
						uint32 mm = CAGE_SHADER_MAX_INSTANCES;
						mm = min(mm, m->getInstancesLimitHint());
						if (m->getSkeletonBones())
							mm = min(mm, CAGE_SHADER_MAX_BONES / m->getSkeletonBones());
						pass->opaques.push_back(detail::systemArena().createHolder<Objects>(m.share(), mm));
						obj = pass->opaques.back().get();
						if (mm > 1)
							opaqueObjectsMap[m.get()] = obj;
					}
					else
					{
						obj = it->second;
						if (obj->uniMeshes.size() + 1 == obj->uniMeshes.capacity())
							opaqueObjectsMap.erase(m.get());
					}
				}
				obj->uniMeshes.emplace_back();
				Objects::UniMesh *sm = &obj->uniMeshes.back();
				sm->color = vec4(e->render.color, e->render.opacity);
				sm->mMat = Mat3x4(model);
				sm->mvpMat = mvp;
				if (any(m->getFlags() & MeshRenderFlags::VelocityWrite))
					sm->mvpPrevMat = mvpPrev;
				else
					sm->mvpPrevMat = mvp;
				sm->normalMat = Mat3x4(inverse(mat3(model)));
				sm->normalMat.data[2][3] = any(m->getFlags() & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled
				if (e->animatedTexture)
					sm->aniTexFrames = detail::evalSamplesForTextureAnimation(obj->textures[CAGE_SHADER_TEXTURE_ALBEDO].get(), dispatchTime, e->animatedTexture->startTime, e->animatedTexture->speed, e->animatedTexture->offset);
				if (!obj->uniArmatures.empty())
				{
					uint32 bonesCount = m->getSkeletonBones();
					Mat3x4 *sa = &obj->uniArmatures[obj->uniMeshes.size() * bonesCount];
					CAGE_ASSERT(!e->animatedSkeleton || e->animatedSkeleton->name);
					bool initialized = false;
					if (e->animatedSkeleton && m->getSkeletonName())
					{
						const auto &ba = *e->animatedSkeleton;
						Holder<SkeletalAnimation> an = engineAssets()->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ba.name);
						Holder<SkeletonRig> skel = engineAssets()->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(m->getSkeletonName());
						if (an && skel)
						{
							CAGE_ASSERT(skel->bonesCount() == bonesCount, skel->bonesCount(), bonesCount);
							real c = detail::evalCoefficientForSkeletalAnimation(an.get(), dispatchTime, ba.startTime, ba.speed, ba.offset);
							skel->animateSkin(an.get(), c, tmpArmature, tmpArmature2);
							for (uint32 i = 0; i < bonesCount; i++)
								sa[i] = Mat3x4(tmpArmature2[i]);
							initialized = true;
						}
					}
					if (!initialized)
					{
						for (uint32 i = 0; i < bonesCount; i++)
							sa[i] = Mat3x4();
					}
				}
			}

			void addRenderableTexts(RenderPassImpl *pass)
			{
				OPTICK_EVENT("texts");
				for (EmitText &e : emitRead->renderableTexts)
				{
					if ((e.renderText.sceneMask & pass->sceneMask) == 0)
						continue;
					if (!e.renderText.font)
						e.renderText.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
					Holder<Font> font = engineAssets()->get<AssetSchemeIndexFont, Font>(e.renderText.font);
					if (!font)
						continue;
					string s = loadInternationalizedText(engineAssets(), e.renderText.assetName, e.renderText.textName, e.renderText.value);
					if (s.empty())
						continue;

					Holder<Texts::Render> r = detail::systemArena().createHolder<Texts::Render>();
					uint32 count = 0;
					font->transcript(s, nullptr, count);
					r->glyphs.resize(count);
					font->transcript(s, r->glyphs.data(), count);
					r->color = e.renderText.color;
					r->format.size = 1;
					vec2 size;
					font->size(r->glyphs.data(), count, r->format, size);
					r->format.wrapWidth = size[0];
					r->transform = pass->viewProj * e.model * mat4(vec3(size * vec2(-0.5, 0.5), 0));

					// todo frustum culling

					Texts *tex = nullptr;
					{
						auto it = textsMap.find(font.get());
						if (it == textsMap.end())
						{
							pass->texts.push_back(detail::systemArena().createHolder<Texts>(font.share()));
							tex = pass->texts.back().get();
							textsMap[font.get()] = tex;
						}
						else
							tex = it->second;
					}

					// add at end
					tex->renders.push_back(templates::move(r));
				}
			}

			void addRenderableLights(RenderPassImpl *pass)
			{
				OPTICK_EVENT("lights");
				for (EmitLight &it : emitRead->lights)
					addLight(pass, &it);
			}

			void addLight(RenderPassImpl *pass, EmitLight *light)
			{
				mat4 mvpMat;
				switch (light->light.lightType)
				{
				case LightTypeEnum::Directional:
					mvpMat = mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -1, -1, 0, 1); // full screen quad
					break;
				case LightTypeEnum::Spot:
					mvpMat = pass->viewProj * light->model * lightSpotCone(lightRange(light->light.color, light->light.attenuation), light->light.spotAngle);
					if (!frustumCulling(meshCone->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				case LightTypeEnum::Point:
					mvpMat = pass->viewProj * light->model * mat4::scale(lightRange(light->light.color, light->light.attenuation));
					if (!frustumCulling(meshSphere->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				}
				addLight(pass->lights, mvpMat, light);
			}

			void addLight(Translucent *trans, const mat4 &mvpMat, EmitLight *light)
			{
				// todo test if the mesh is in range of the light
				addLight(trans->lights, mvpMat, light);
			}

			void addLight(std::vector<Holder<Lights>> &lights, const mat4 &mvpMat, EmitLight *light)
			{
				Lights *lig = nullptr;
				if (light->shadowmap)
				{
					lights.push_back(detail::systemArena().createHolder<Lights>(light->light.lightType, light->shadowmap->index, 1));
					lig = lights.back().get();
				}
				else
				{
					for (const Holder<Lights> &it : lights)
					{
						if (it->uniLights.size() == it->uniLights.capacity())
							continue;
						if (it->lightType != light->light.lightType)
							continue;
						if (it->shadowmap != 0)
							continue;
						lig = it.get();
						break;
					}
					if (!lig)
					{
						lights.insert(lights.begin(), detail::systemArena().createHolder<Lights>(light->light.lightType, 0, CAGE_SHADER_MAX_INSTANCES));
						lig = lights.front().get();
					}
				}
				lig->uniLights.emplace_back();
				Lights::UniLight *sl = &lig->uniLights.back();
				// todo this struct could be precomputed
				sl->mvpMat = mvpMat;
				sl->color = vec4(light->light.color, cos(light->light.spotAngle * 0.5));
				sl->attenuation = vec4(light->light.attenuation, light->light.spotExponent);
				sl->direction = vec4(normalize(vec3(light->model * vec4(0, 0, -1, 0))), 0);
				sl->position = light->model * vec4(0, 0, 0, 1);
				if (light->shadowmap)
					sl->shadowMat = light->shadowmap->shadowMat;
			}

			explicit GraphicsPrepareImpl(const EngineCreateConfig &config) : emitBufferA(config), emitBufferB(config), emitBufferC(config), emitBuffers{ &emitBufferA, &emitBufferB, &emitBufferC }
			{
				SwapBufferGuardCreateConfig cfg(3);
				cfg.repeatedReads = true;
				swapController = newSwapBufferGuard(cfg);
			}

			void finalize()
			{
				meshSphere.clear();
				meshCone.clear();
				graphicsDispatch->renderPasses.clear();
			}

			void emitTransform(EmitTransforms *c, Entity *e)
			{
				c->current = e->value<TransformComponent>(TransformComponent::component);
				if (e->has(TransformComponent::componentHistory))
					c->history = e->value<TransformComponent>(TransformComponent::componentHistory);
				else
					c->history = c->current;
			}

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping graphics emit");
					return;
				}

				emitWrite = emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitWrite(emitWrite);
				emitWrite->renderableObjects.clear();
				emitWrite->renderableTexts.clear();
				emitWrite->lights.clear();
				emitWrite->cameras.clear();
				emitWrite->time = time;
				emitWrite->fresh = true;

				// emit renderable objects
				for (Entity *e : RenderComponent::component->entities())
				{
					EmitRender c;
					emitTransform(&c, e);
					c.render = e->value<RenderComponent>(RenderComponent::component);
					if (e->has(TextureAnimationComponent::component))
						c.animatedTexture = detail::systemArena().createHolder<TextureAnimationComponent>(e->value<TextureAnimationComponent>(TextureAnimationComponent::component));
					if (e->has(SkeletalAnimationComponent::component))
						c.animatedSkeleton = detail::systemArena().createHolder<SkeletalAnimationComponent>(e->value<SkeletalAnimationComponent>(SkeletalAnimationComponent::component));
					emitWrite->renderableObjects.push_back(templates::move(c));
				}

				// emit renderable texts
				for (Entity *e : TextComponent::component->entities())
				{
					EmitText c;
					emitTransform(&c, e);
					c.renderText = e->value<TextComponent>(TextComponent::component);
					emitWrite->renderableTexts.push_back(templates::move(c));
				}

				// emit lights
				for (Entity *e : LightComponent::component->entities())
				{
					EmitLight c;
					emitTransform(&c, e);
					c.history.scale = c.current.scale = 1;
					c.light = e->value<LightComponent>(LightComponent::component);
					if (e->has(ShadowmapComponent::component))
						c.shadowmap = detail::systemArena().createHolder<ShadowmapImpl>(e->value<ShadowmapComponent>(ShadowmapComponent::component));
					c.entityId = ((uintPtr)e) ^ e->name();
					emitWrite->lights.push_back(templates::move(c));
				}

				// emit cameras
				CameraEffectsFlags effectsMask = ~CameraEffectsFlags::None;
				if (confNoAmbientOcclusion)
					effectsMask &= ~CameraEffectsFlags::AmbientOcclusion;
				if (confNoBloom)
					effectsMask &= ~CameraEffectsFlags::Bloom;
				if (confNoMotionBlur)
					effectsMask &= ~CameraEffectsFlags::MotionBlur;
				for (Entity *e : CameraComponent::component->entities())
				{
					EmitCamera c;
					emitTransform(&c, e);
					c.history.scale = c.current.scale = 1;
					c.camera = e->value<CameraComponent>(CameraComponent::component);
					c.camera.effects &= effectsMask;
					c.entityId = ((uintPtr)e) ^ e->name();
					emitWrite->cameras.push_back(templates::move(c));
				}
			}

			void updateDefaultValues(EmitRender *e)
			{
				if (!e->render.object)
					return;

				Holder<RenderObject> o = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(e->render.object);

				if (!o && !engineAssets()->tryGet<AssetSchemeIndexMesh, Mesh>(e->render.object))
				{
					if (!confRenderMissingMeshes)
					{
						e->render.object = 0; // disable rendering further in the pipeline
						return;
					}
					e->render.object = HashString("cage/mesh/fake.obj");
				}

				if (o)
				{
					if (!e->render.color.valid())
						e->render.color = o->color;
					if (!e->render.opacity.valid())
						e->render.opacity = o->opacity;

					{
						Holder<TextureAnimationComponent> &c = e->animatedTexture;
						if (!c && (o->texAnimSpeed.valid() || o->texAnimOffset.valid()))
							c = detail::systemArena().createHolder<TextureAnimationComponent>();
						if (c)
						{
							if (!c->speed.valid())
								c->speed = o->texAnimSpeed;
							if (!c->offset.valid())
								c->offset = o->texAnimOffset;
						}
					}

					{
						Holder<SkeletalAnimationComponent> &c = e->animatedSkeleton;
						if (!c && o->skelAnimName)
							c = detail::systemArena().createHolder<SkeletalAnimationComponent>();
						if (c)
						{
							if (!c->name)
								c->name = o->skelAnimName;
							if (!c->speed.valid())
								c->speed = o->skelAnimSpeed;
							if (!c->offset.valid())
								c->offset = o->skelAnimOffset;
						}
					}
				}

				if (!e->render.color.valid())
					e->render.color = vec3(0);
				if (!e->render.opacity.valid())
					e->render.opacity = 1;

				if (e->animatedTexture)
				{
					TextureAnimationComponent *c = e->animatedTexture.get();
					if (!c->speed.valid())
						c->speed = 1;
					if (!c->offset.valid())
						c->offset = 0;
				}

				if (e->animatedSkeleton)
				{
					if (e->animatedSkeleton->name)
					{
						SkeletalAnimationComponent *c = e->animatedSkeleton.get();
						if (!c->speed.valid())
							c->speed = 1;
						if (!c->offset.valid())
							c->offset = 0;
					}
					else
						e->animatedSkeleton.clear();
				}
			}

			void tick(uint64 time)
			{
				auto lock = swapController->read();
				if (!lock)
				{
					CAGE_LOG_DEBUG(SeverityEnum::Warning, "engine", "skipping graphics prepare");
					return;
				}

				AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return;

				meshSphere = ass->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/sphere.obj"));
				meshCone = ass->get<AssetSchemeIndexMesh, Mesh>(HashString("cage/mesh/cone.obj"));

				emitRead = emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitRead(emitRead);

				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, controlThread().updatePeriod());
				elapsedDispatchTime = dispatchTime - lastDispatchTime;
				lastDispatchTime = dispatchTime;
				shm2d = shmCube = 0;

				graphicsDispatch->renderPasses.clear();

				ivec2 resolution = engineWindow()->resolution();
				graphicsDispatch->windowWidth = numeric_cast<uint32>(resolution.x);
				graphicsDispatch->windowHeight = numeric_cast<uint32>(resolution.y);
				if (graphicsDispatch->windowWidth == 0 || graphicsDispatch->windowHeight == 0)
					return;

				if (emitRead->fresh)
				{ // update default values
					OPTICK_EVENT("update default values");
					for (EmitRender &it : emitRead->renderableObjects)
						updateDefaultValues(&it);
					emitRead->fresh = false;
				}

				{ // update model matrices
					OPTICK_EVENT("update model matrices");
					real interFactor = clamp(real(dispatchTime - emitTime) / controlThread().updatePeriod(), 0, 1);
					//CAGE_LOG(SeverityEnum::Info, "timing", stringizer() + "emit: " + emitTime + ", current: " + time + ", dispatch: " + dispatchTime + ", interpolate: " + interFactor);
					for (auto &it : emitRead->renderableObjects)
						it.updateModelMatrix(interFactor);
					for (auto &it : emitRead->renderableTexts)
						it.updateModelMatrix(interFactor);
					for (auto &it : emitRead->lights)
						it.updateModelMatrix(interFactor);
					for (auto &it : emitRead->cameras)
						it.updateModelMatrix(interFactor);
				}

				// generate shadowmap render passes
				for (auto &it : emitRead->lights)
				{
					if (!it.shadowmap)
						continue;
					initializeRenderPassForShadowmap(newRenderPass(), &it);
				}

				{ // generate camera render passes
					std::sort(emitRead->cameras.begin(), emitRead->cameras.end(), [](const EmitCamera &a, const EmitCamera &b)
					{
						CAGE_ASSERT(a.camera.cameraOrder != b.camera.cameraOrder, a.camera.cameraOrder);
						return a.camera.cameraOrder < b.camera.cameraOrder;
					});
					for (auto &it : emitRead->cameras)
					{
						if (graphicsPrepareThread().stereoMode == StereoModeEnum::Mono || it.camera.target)
						{ // mono
							initializeRenderPassForCamera(newRenderPass(), &it, StereoEyeEnum::Mono);
						}
						else
						{ // stereo
							initializeRenderPassForCamera(newRenderPass(), &it, StereoEyeEnum::Left);
							initializeRenderPassForCamera(newRenderPass(), &it, StereoEyeEnum::Right);
						}
					}
				}
			}
		};

		GraphicsPrepareImpl *graphicsPrepare;
	}

	Mat3x4::Mat3x4() : Mat3x4(mat4())
	{}

	Mat3x4::Mat3x4(const mat4 &in)
	{
		CAGE_ASSERT(in[3] == 0 && in[7] == 0 && in[11] == 0 && in[15] == 1, in);
		for (uint32 a = 0; a < 4; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 4 + b];
	}

	Mat3x4::Mat3x4(const mat3 &in)
	{
		for (uint32 a = 0; a < 3; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 3 + b];
	}

	void ShaderConfig::set(uint32 name, uint32 value)
	{
		CAGE_ASSERT(name < CAGE_SHADER_MAX_ROUTINES);
		shaderRoutines[name] = value;
	}

	Objects::Objects(Holder<Mesh> mesh_, uint32 max) : mesh(templates::move(mesh_))
	{
		AssetManager *ass = engineAssets();
		uniMeshes.reserve(max);
		if (mesh->getSkeletonBones())
			uniArmatures.resize(mesh->getSkeletonBones() * max);
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			uint32 n = mesh->getTextureName(i);
			textures[i] = n ? ass->get<AssetSchemeIndexTexture, Texture>(n) : Holder<Texture>();
		}
		shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_SKELETON, mesh->getSkeletonBones() > 0 ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING);

		if (textures[CAGE_SHADER_TEXTURE_ALBEDO])
		{
			switch (textures[CAGE_SHADER_TEXTURE_ALBEDO]->getTarget())
			{
			case GL_TEXTURE_2D_ARRAY:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY);
				break;
			case GL_TEXTURE_CUBE_MAP:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE);
				break;
			default:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D);
				break;
			}
		}
		else
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);

		if (textures[CAGE_SHADER_TEXTURE_SPECIAL])
		{
			switch (textures[CAGE_SHADER_TEXTURE_SPECIAL]->getTarget())
			{
			case GL_TEXTURE_2D_ARRAY:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY);
				break;
			case GL_TEXTURE_CUBE_MAP:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE);
				break;
			default:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D);
				break;
			}
		}
		else
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);

		if (textures[CAGE_SHADER_TEXTURE_NORMAL] && !confNoNormalMap)
		{
			switch (textures[CAGE_SHADER_TEXTURE_NORMAL]->getTarget())
			{
			case GL_TEXTURE_2D_ARRAY:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY);
				break;
			case GL_TEXTURE_CUBE_MAP:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE);
				break;
			default:
				shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D);
				break;
			}
		}
		else
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
	}

	Lights::Lights(LightTypeEnum lightType, sint32 shadowmap, uint32 max) : shadowmap(shadowmap), lightType(lightType)
	{
		uniLights.reserve(max);
		switch (lightType)
		{
		case LightTypeEnum::Directional:
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW);
			break;
		case LightTypeEnum::Spot:
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOT : CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW);
			break;
		case LightTypeEnum::Point:
			shaderConfig.set(CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINT : CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW);
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid light type");
		}
	}

	Translucent::Translucent(Holder<Mesh> mesh_) : object(templates::move(mesh_), 1)
	{}

	Texts::Texts(Holder<Font> font_) : font(templates::move(font_))
	{}

	void graphicsPrepareCreate(const EngineCreateConfig &config)
	{
		graphicsPrepare = detail::systemArena().createObject<GraphicsPrepareImpl>(config);
	}

	void graphicsPrepareDestroy()
	{
		detail::systemArena().destroy<GraphicsPrepareImpl>(graphicsPrepare);
		graphicsPrepare = nullptr;
	}

	void graphicsPrepareFinalize()
	{
		graphicsPrepare->finalize();
	}

	void graphicsPrepareEmit(uint64 time)
	{
		graphicsPrepare->emit(time);
	}

	void graphicsPrepareTick(uint64 time)
	{
		graphicsPrepare->tick(time);
	}
}
