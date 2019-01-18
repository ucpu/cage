#include <algorithm>
#include <vector>
#include <unordered_map>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/hashString.h>
#include <cage-core/color.h>
#include <cage-core/swapBufferController.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/engine.h>
#include <cage-client/opengl.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/stereoscopy.h>
#include <cage-client/window.h>

#include "../engine.h"
#include "graphics.h"

namespace cage
{
	// implemented in gui
	string loadInternationalizedText(assetManagerClass *assets, uint32 asset, uint32 text, string params);

	namespace
	{
		configBool renderMissingMeshes("cage-client.engine.renderMissingMeshes", false);
		configBool renderSkeletonBones("cage-client.engine.renderSkeletonBones", false);

		struct shadowmapImpl : public shadowmapComponent
		{
			mat4 shadowMat;
			sint32 index;
			shadowmapImpl(const shadowmapComponent &other) : shadowmapComponent(other), index(0) {}
		};

		struct emitTransformsStruct
		{
			transformComponent transform, transformHistory;
			mat4 model;
			mat4 modelPrev;
			void updateModelMatrix(real interFactor)
			{
				model = mat4(interpolate(transformHistory, transform, interFactor));
				modelPrev = mat4(interpolate(transformHistory, transform, interFactor - 0.2));
			}
		};

		struct emitRenderObjectStruct : public emitTransformsStruct
		{
			renderComponent render;
			animatedTextureComponent *animatedTexture;
			animatedSkeletonComponent *animatedSkeleton;
			emitRenderObjectStruct()
			{
				detail::memset(this, 0, sizeof(emitRenderObjectStruct));
			}
		};

		struct emitRenderTextStruct : public emitTransformsStruct
		{
			renderTextComponent renderText;
			emitRenderTextStruct()
			{
				detail::memset(this, 0, sizeof(emitRenderTextStruct));
			}
		};

		struct emitLightStruct : public emitTransformsStruct
		{
			lightComponent light;
			shadowmapImpl *shadowmap;
			uintPtr entityId;
			emitLightStruct()
			{
				detail::memset(this, 0, sizeof(emitLightStruct));
			}
		};

		struct emitCameraStruct : public emitTransformsStruct
		{
			cameraComponent camera;
			uintPtr entityId;
			emitCameraStruct()
			{
				detail::memset(this, 0, sizeof(emitCameraStruct));
			}
		};

		struct renderPassImpl : public renderPassStruct
		{
			mat4 viewProjPrev;
			uint32 renderMask;
			real lodSelection; // vertical size of screen, at distance of one world-space-unit from camera, in pixels
			bool lodOrthographic;
			renderPassImpl()
			{
				detail::memset(this, 0, sizeof(renderPassImpl));
			}
		};

		struct emitStruct
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> emitMemory;
			memoryArena emitArena;

			std::vector<emitRenderObjectStruct*> renderableObjects;
			std::vector<emitRenderTextStruct*> renderableTexts;
			std::vector<emitLightStruct*> lights;
			std::vector<emitCameraStruct*> cameras;

			uint64 time;

			emitStruct(const engineCreateConfig &config) : emitMemory(config.graphicsEmitMemory), emitArena(&emitMemory), time(0)
			{
				renderableObjects.reserve(256);
				renderableTexts.reserve(64);
				lights.reserve(32);
				cameras.reserve(4);
			}

			~emitStruct()
			{
				emitArena.flush();
			}
		};

		struct graphicsPrepareImpl
		{
			emitStruct emitBufferA, emitBufferB, emitBufferC; // this is awfully stupid, damn you c++
			emitStruct *emitBuffers[3];
			emitStruct *emitRead, *emitWrite;
			holder<swapBufferControllerClass> swapController;

			mat4 tmpArmature[CAGE_SHADER_MAX_BONES];
			mat4 tmpArmature2[CAGE_SHADER_MAX_BONES];

			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> dispatchMemory;
			memoryArena dispatchArena;

			interpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			uint64 lastDispatchTime;
			uint64 elapsedDispatchTime;
			sint32 shm2d, shmCube;

			typedef std::unordered_map<meshClass*, objectsStruct*> opaqueObjectsMapType;
			opaqueObjectsMapType opaqueObjectsMap;
			typedef std::unordered_map<fontClass*, textsStruct*> textsMapType;
			textsMapType textsMap;

			static real lightRange(const vec3 &color, const vec3 &attenuation)
			{
				real c = max(color[0], max(color[1], color[2]));
				if (c <= 1e-5)
					return 0;
				real e = c * 256.f / 3.f;
				real x = attenuation[0], y = attenuation[1], z = attenuation[2];
				if (z < 1e-5)
				{
					CAGE_ASSERT_RUNTIME(y > 1e-5, "invalid light attenuation", attenuation);
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

			static void setShaderRoutine(shaderConfigStruct *s, uint32 stage, uint32 name, uint32 value)
			{
				for (uint32 i = 0; i < MaxRoutines; i++)
				{
					if (s->shaderRoutineStage[i])
						continue;
					s->shaderRoutineStage[i] = stage;
					s->shaderRoutineName[i] = name;
					s->shaderRoutineValue[i] = value;
					return;
				}
				CAGE_THROW_CRITICAL(exception, "shader routines over limit");
			}

			renderPassImpl *newRenderPass()
			{
				opaqueObjectsMap.clear();
				textsMap.clear();
				renderPassImpl *t = dispatchArena.createObject<renderPassImpl>();
				if (graphicsDispatch->firstRenderPass)
					graphicsDispatch->lastRenderPass->next = t;
				else
					graphicsDispatch->firstRenderPass = t;
				graphicsDispatch->lastRenderPass = t;
				return t;
			}

			static real fovToLodSelection(rads fov)
			{
				return tan(fov * 0.5) * 2;
			}

			static void initializeStereoCamera(renderPassImpl *pass, emitCameraStruct *camera, eyeEnum eye, const mat4 &model)
			{
				real x = camera->camera.viewportOrigin[0], y = camera->camera.viewportOrigin[1], w = camera->camera.viewportSize[0], h = camera->camera.viewportSize[1];
				stereoCameraStruct cam;
				cam.position = vec3(model * vec4(0, 0, 0, 1));
				cam.direction = vec3(model * vec4(0, 0, -1, 0));
				cam.worldUp = vec3(model * vec4(0, 1, 0, 0));
				cam.fov = camera->camera.camera.perspectiveFov;
				cam.near = camera->camera.near;
				cam.far = camera->camera.far;
				cam.zeroParallaxDistance = camera->camera.zeroParallaxDistance;
				cam.eyeSeparation = camera->camera.eyeSeparation;
				cam.ortographic = camera->camera.cameraType == cameraTypeEnum::Orthographic;
				stereoscopy(eye, cam, real(graphicsDispatch->windowWidth) / real(graphicsDispatch->windowHeight), (stereoModeEnum)graphicsPrepareThread().stereoMode, pass->view, pass->proj, x, y, w, h);
				if (camera->camera.cameraType == cameraTypeEnum::Perspective)
					pass->lodSelection = fovToLodSelection(camera->camera.camera.perspectiveFov) * graphicsDispatch->windowHeight;
				else
				{
					const vec2 &os = camera->camera.camera.orthographicSize;
					pass->proj = orthographicProjection(-os[0], os[0], -os[1], os[1], camera->camera.near, camera->camera.far);
					pass->lodSelection = os[1] * graphicsDispatch->windowHeight;
					pass->lodOrthographic = true;
				}
				pass->viewProj = pass->proj * pass->view;
				pass->vpX = numeric_cast<uint32>(x * real(graphicsDispatch->windowWidth));
				pass->vpY = numeric_cast<uint32>(y * real(graphicsDispatch->windowHeight));
				pass->vpW = numeric_cast<uint32>(w * real(graphicsDispatch->windowWidth));
				pass->vpH = numeric_cast<uint32>(h * real(graphicsDispatch->windowHeight));
			}

			static void initializeTargetCamera(renderPassImpl *pass, const mat4 &model)
			{
				vec3 p = vec3(model * vec4(0, 0, 0, 1));
				pass->view = lookAt(p, p + vec3(model * vec4(0, 0, -1, 0)), vec3(model * vec4(0, 1, 0, 0)));
				pass->viewProj = pass->proj * pass->view;
			}

			void initializeRenderPassForCamera(renderPassImpl *pass, emitCameraStruct *camera, eyeEnum eye)
			{
				if (camera->camera.target)
				{
					uint32 w = 0, h = 0;
					camera->camera.target->getResolution(w, h);
					if (w == 0 || h == 0)
						CAGE_THROW_ERROR(exception, "target texture has zero resolution");
					switch (camera->camera.cameraType)
					{
					case cameraTypeEnum::Orthographic:
					{
						const vec2 &os = camera->camera.camera.orthographicSize;
						pass->proj = orthographicProjection(-os[0], os[0], -os[1], os[1], camera->camera.near, camera->camera.far);
						pass->lodSelection = os[1] * h;
						pass->lodOrthographic = true;
					} break;
					case cameraTypeEnum::Perspective:
						pass->proj = perspectiveProjection(camera->camera.camera.perspectiveFov, real(w) / real(h), camera->camera.near, camera->camera.far);
						pass->lodSelection = fovToLodSelection(camera->camera.camera.perspectiveFov) * h;
						break;
					default:
						CAGE_THROW_ERROR(exception, "invalid camera type");
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
					initializeStereoCamera(pass, camera, eye, camera->modelPrev);
					pass->viewProjPrev = pass->viewProj;
					initializeStereoCamera(pass, camera, eye, camera->model);
				}
				pass->shaderViewport.vpInv = pass->viewProj.inverse();
				pass->shaderViewport.eyePos = camera->model * vec4(0, 0, 0, 1);
				pass->shaderViewport.ambientLight = vec4(camera->camera.ambientLight, 0);
				pass->shaderViewport.viewport = vec4(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				pass->targetTexture = camera->camera.target;
				pass->clearFlags = ((camera->camera.clear & cameraClearFlags::Color) == cameraClearFlags::Color ? GL_COLOR_BUFFER_BIT : 0) | ((camera->camera.clear & cameraClearFlags::Depth) == cameraClearFlags::Depth ? GL_DEPTH_BUFFER_BIT : 0);
				pass->renderMask = camera->camera.renderMask;
				pass->entityId = camera->entityId;
				(cameraEffectsStruct&)*pass = (cameraEffectsStruct&)camera->camera;
				real eyeAdaptationSpeed = real(elapsedDispatchTime) * 1e-6;
				pass->eyeAdaptation.darkerSpeed *= eyeAdaptationSpeed;
				pass->eyeAdaptation.lighterSpeed *= eyeAdaptationSpeed;
				addRenderableObjects(pass, false);
				addRenderableTexts(pass);
				for (auto it : emitRead->lights)
					addLight(pass, it);
			}

			void initializeRenderPassForShadowmap(renderPassImpl *pass, emitLightStruct *light)
			{
				pass->view = light->model.inverse();
				switch (light->light.lightType)
				{
				case lightTypeEnum::Directional:
					pass->proj = orthographicProjection(-light->shadowmap->worldSize[0], light->shadowmap->worldSize[0], -light->shadowmap->worldSize[1], light->shadowmap->worldSize[1], -light->shadowmap->worldSize[2], light->shadowmap->worldSize[2]);
					pass->lodSelection = light->shadowmap->worldSize[1] * 2;
					pass->lodOrthographic = true;
					break;
				case lightTypeEnum::Spot:
					pass->proj = perspectiveProjection(light->light.spotAngle, 1, light->shadowmap->worldSize[0], light->shadowmap->worldSize[1]);
					pass->lodSelection = fovToLodSelection(light->light.spotAngle) * light->shadowmap->resolution;
					break;
				case lightTypeEnum::Point:
					pass->proj = perspectiveProjection(degs(90), 1, light->shadowmap->worldSize[0], light->shadowmap->worldSize[1]);
					pass->lodSelection = fovToLodSelection(degs(90)) * light->shadowmap->resolution;
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid light type");
				}
				pass->viewProj = pass->proj * pass->view;
				pass->viewProjPrev = pass->viewProj;
				pass->shadowmapResolution = pass->vpW = pass->vpH = light->shadowmap->resolution;
				pass->renderMask = 0xffffffff;
				pass->targetShadowmap = light->light.lightType == lightTypeEnum::Point ? (-shmCube++ - 1) : (shm2d++ + 1);
				light->shadowmap->index = pass->targetShadowmap;
				static const mat4 bias = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				light->shadowmap->shadowMat = bias * pass->viewProj;
				pass->entityId = light->entityId;
				addRenderableObjects(pass, true);
			}

			void addRenderableObjects(renderPassImpl *pass, bool shadows)
			{
				CAGE_ASSERT_RUNTIME(pass->lodSelection > 0);
				for (emitRenderObjectStruct *e : emitRead->renderableObjects)
				{
					if ((e->render.renderMask & pass->renderMask) == 0)
						continue;
					if (!assets()->ready(e->render.object))
					{
						if (renderMissingMeshes)
							addRenderableMesh(pass, e, graphicsDispatch->meshFake);
						continue;
					}
					switch (assets()->scheme(e->render.object))
					{
					case assetSchemeIndexMesh:
					{
						meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(e->render.object);
						addRenderableMesh(pass, e, m);
					} break;
					case assetSchemeIndexObject:
					{
						objectClass *o = assets()->get<assetSchemeIndexObject, objectClass>(e->render.object);
						if (o->lodsCount() == 0)
							continue;
						if (shadows && o->shadower)
						{
							CAGE_ASSERT_RUNTIME(assets()->ready(o->shadower), e->render.object, o->shadower);
							meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(o->shadower);
							addRenderableMesh(pass, e, m);
							continue;
						}
						uint32 lod = 0;
						if (o->lodsCount() > 1)
						{
							real d = 1;
							if (!pass->lodOrthographic)
							{
								vec4 ep4 = e->model * vec4(0, 0, 0, 1);
								d = (vec3(ep4) / ep4[3]).distance(vec3(pass->shaderViewport.eyePos));
							}
							real f = pass->lodSelection * o->worldSize / (d * o->pixelsSize);
							lod = o->lodSelect(f.value);
						}
						for (uint32 msh : o->meshes(lod))
						{
							meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(msh);
							addRenderableMesh(pass, e, m);
						}
					} break;
					default:
						CAGE_THROW_CRITICAL(exception, "trying to render an invalid-scheme asset");
					}
				}
			}

			void addRenderableSkeleton(renderPassImpl *pass, emitRenderObjectStruct *e, skeletonClass *s, const mat4 &model, const mat4 &mvp)
			{
				uint32 bonesCount = s->bonesCount();
				if (e->animatedSkeleton && assets()->ready(e->animatedSkeleton->name))
				{
					CAGE_ASSERT_RUNTIME(s->bonesCount() == bonesCount, s->bonesCount(), bonesCount);
					const auto &ba = *e->animatedSkeleton;
					animationClass *an = assets()->get<assetSchemeIndexAnimation, animationClass>(ba.name);
					real c = detail::evalCoefficientForSkeletalAnimation(an, dispatchTime, ba.startTime, ba.speed, ba.offset);
					s->animateSkeleton(an, c, tmpArmature, tmpArmature2);
				}
				else
				{
					for (uint32 i = 0; i < bonesCount; i++)
						tmpArmature2[i] = mat4();
				}
				meshClass *mesh = assets()->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/bone.obj"));
				CAGE_ASSERT_RUNTIME(mesh->getSkeletonName() == 0);
				for (uint32 i = 0; i < bonesCount; i++)
				{
					e->render.color = convertHsvToRgb(vec3(real(i) / real(bonesCount), 1, 1));
					mat4 m = model * tmpArmature2[i];
					mat4 mvp = pass->viewProj * m;
					addRenderableMesh(pass, e, mesh, m, mvp, mvp);
				}
			}

			void addRenderableMesh(renderPassImpl *pass, emitRenderObjectStruct *e, meshClass *m)
			{
				addRenderableMesh(pass, e, m, e->model, pass->viewProj * e->model, pass->viewProjPrev * e->modelPrev);
			}

			void addRenderableMesh(renderPassImpl *pass, emitRenderObjectStruct *e, meshClass *m, const mat4 &model, const mat4 &mvp, const mat4 &mvpPrev)
			{
				if (!frustumCulling(m->getBoundingBox(), mvp))
					return;
				if (pass->targetShadowmap != 0 && (m->getFlags() & meshFlags::ShadowCast) == meshFlags::None)
					return;
				if (m->getSkeletonName() != 0 && renderSkeletonBones)
				{
					skeletonClass *s = assets()->get<assetSchemeIndexSkeleton, skeletonClass>(m->getSkeletonName());
					addRenderableSkeleton(pass, e, s, model, mvp);
					return;
				}
				objectsStruct *obj = nullptr;
				if ((m->getFlags() & meshFlags::Translucency) == meshFlags::Translucency)
				{ // translucent
					translucentStruct *t = dispatchArena.createObject<translucentStruct>(m);
					obj = &t->object;
					// add at end
					if (pass->lastTranslucent)
						pass->lastTranslucent->next = t;
					else
						pass->firstTranslucent = t;
					pass->lastTranslucent = t;
				}
				else
				{ // opaque
					auto it = opaqueObjectsMap.find(m);
					if (it == opaqueObjectsMap.end())
					{
						uint32 mm = CAGE_SHADER_MAX_INSTANCES;
						if (m->getSkeletonBones())
							mm = min(mm, CAGE_SHADER_MAX_BONES / m->getSkeletonBones());
						obj = dispatchArena.createObject<objectsStruct>(m, mm);
						// add at end
						if (pass->lastOpaque)
							pass->lastOpaque->next = obj;
						else
							pass->firstOpaque = obj;
						pass->lastOpaque = obj;
						if (mm > 1)
							opaqueObjectsMap[m] = obj;
					}
					else
					{
						obj = it->second;
						CAGE_ASSERT_RUNTIME(obj->count < obj->max);
						if (obj->count + 1 == obj->max)
							opaqueObjectsMap.erase(m);
					}
				}
				objectsStruct::shaderMeshStruct *sm = obj->shaderMeshes + obj->count;
				sm->color = vec4(e->render.color, 0);
				sm->mMat = model;
				sm->mvpMat = mvp;
				if ((m->getFlags() & meshFlags::VelocityWrite) == meshFlags::VelocityWrite)
					sm->mvpPrevMat = mvpPrev;
				else
					sm->mvpPrevMat = mvp;
				sm->normalMat = mat3(model).inverse();
				sm->normalMat.data[2][3] = ((m->getFlags() & meshFlags::Lighting) == meshFlags::Lighting) ? 1 : 0; // is ligting enabled
				if (e->animatedTexture)
					sm->aniTexFrames = detail::evalSamplesForTextureAnimation(obj->textures[CAGE_SHADER_TEXTURE_ALBEDO], dispatchTime, e->animatedTexture->startTime, e->animatedTexture->speed, e->animatedTexture->offset);
				if (obj->shaderArmatures)
				{
					uint32 bonesCount = m->getSkeletonBones();
					mat3x4 *sa = obj->shaderArmatures + obj->count * bonesCount;
					if (e->animatedSkeleton && m->getSkeletonName() != 0 && assets()->ready(e->animatedSkeleton->name))
					{
						skeletonClass *skel = assets()->get<assetSchemeIndexSkeleton, skeletonClass>(m->getSkeletonName());
						CAGE_ASSERT_RUNTIME(skel->bonesCount() == bonesCount, skel->bonesCount(), bonesCount);
						const auto &ba = *e->animatedSkeleton;
						animationClass *an = assets()->get<assetSchemeIndexAnimation, animationClass>(ba.name);
						real c = detail::evalCoefficientForSkeletalAnimation(an, dispatchTime, ba.startTime, ba.speed, ba.offset);
						skel->animateSkin(an, c, tmpArmature, tmpArmature2);
						for (uint32 i = 0; i < bonesCount; i++)
							sa[i] = tmpArmature2[i];
					}
					else
					{
						for (uint32 i = 0; i < bonesCount; i++)
							sa[i] = mat4();
					}
				}
				obj->count++;
			}

			void addRenderableTexts(renderPassImpl *pass)
			{
				for (emitRenderTextStruct *e : emitRead->renderableTexts)
				{
					if ((e->renderText.renderMask & pass->renderMask) == 0)
						continue;
					if (!e->renderText.font)
						e->renderText.font = hashString("cage/font/ubuntu/Ubuntu-R.ttf");
					if (!assets()->ready(e->renderText.font))
						continue;
					string s = loadInternationalizedText(assets(), e->renderText.assetName, e->renderText.textName, e->renderText.value);
					if (s.empty())
						continue;

					fontClass *font = assets()->get<assetSchemeIndexFont, fontClass>(e->renderText.font);
					textsStruct::renderStruct *r = dispatchArena.createObject<textsStruct::renderStruct>();
					font->transcript(s, nullptr, r->count);
					r->glyphs = (uint32*)dispatchArena.allocate(r->count * sizeof(uint32), sizeof(uint32));
					font->transcript(s, r->glyphs, r->count);
					r->color = e->renderText.color;
					r->format.size = 1;
					vec2 size;
					font->size(r->glyphs, r->count, r->format, size);
					r->format.wrapWidth = size[0];
					r->transform = pass->viewProj * e->model * mat4(vec3(size * vec2(-0.5, 0.5), 0));

					// todo frustum culling

					textsStruct *tex = nullptr;
					{
						auto it = textsMap.find(font);
						if (it == textsMap.end())
						{
							tex = dispatchArena.createObject<textsStruct>();
							tex->font = font;
							// add at end
							if (pass->lastText)
								pass->lastText->next = tex;
							else
								pass->firstText = tex;
							pass->lastText = tex;
							textsMap[font] = tex;
						}
						else
							tex = it->second;
					}

					// add at end
					if (tex->lastRender)
						tex->lastRender->next = r;
					else
						tex->firtsRender = r;
					tex->lastRender = r;
				}
			}

			void addLight(renderPassImpl *pass, emitLightStruct *e)
			{
				mat4 mvpMat;
				switch (e->light.lightType)
				{
				case lightTypeEnum::Directional:
					mvpMat = mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -1, -1, 0, 1); // full screen quad
					break;
				case lightTypeEnum::Spot:
					mvpMat = pass->viewProj * e->model * lightSpotCone(lightRange(e->light.color, e->light.attenuation), e->light.spotAngle);
					if (!frustumCulling(graphicsDispatch->meshCone->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				case lightTypeEnum::Point:
					mvpMat = pass->viewProj * e->model * mat4(lightRange(e->light.color, e->light.attenuation));
					if (!frustumCulling(graphicsDispatch->meshSphere->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				}
				lightsStruct *lig = nullptr;
				if (e->shadowmap)
				{
					lig = dispatchArena.createObject<lightsStruct>(e->light.lightType, e->shadowmap->index, 1);
					// add at end
					if (pass->lastLighting)
						pass->lastLighting->next = lig;
					else
						pass->firstLighting = lig;
					pass->lastLighting = lig;
					lig->shaderLights[0].shadowMat = e->shadowmap->shadowMat;
				}
				else
				{
					for (lightsStruct *it = pass->firstLighting; it; it = it->next)
					{
						if (it->count == it->max)
							continue;
						if (it->lightType != e->light.lightType)
							continue;
						if (it->shadowmap != 0)
							continue;
						lig = it;
					}
					if (!lig)
					{
						lig = dispatchArena.createObject<lightsStruct>(e->light.lightType, 0, CAGE_SHADER_MAX_INSTANCES);
						// add at begin
						if (pass->firstLighting)
							lig->next = pass->firstLighting;
						else
							pass->lastLighting = lig;
						pass->firstLighting = lig;
					}
				}
				lightsStruct::shaderLightStruct *sl = lig->shaderLights + lig->count;
				sl->mvpMat = mvpMat;
				sl->color = vec4(e->light.color, cos(e->light.spotAngle * 0.5));
				sl->attenuation = vec4(e->light.attenuation, e->light.spotExponent);
				sl->direction = vec4(vec3(e->model * vec4(0, 0, -1, 0)).normalize(), 0);
				sl->position = e->model * vec4(0, 0, 0, 1); sl->position /= sl->position[3];
				lig->count++;
			}

			graphicsPrepareImpl(const engineCreateConfig &config) : emitBufferA(config), emitBufferB(config), emitBufferC(config), emitBuffers{ &emitBufferA, &emitBufferB, &emitBufferC }, emitRead(nullptr), emitWrite(nullptr), dispatchMemory(config.graphicsDispatchMemory), dispatchArena(&dispatchMemory), emitTime(0), dispatchTime(0), lastDispatchTime(0), elapsedDispatchTime(0)
			{
				swapBufferControllerCreateConfig cfg(3);
				cfg.repeatedReads = true;
				swapController = newSwapBufferController(cfg);
			}

			~graphicsPrepareImpl()
			{
				dispatchArena.flush();
			}

			void emitTransform(emitTransformsStruct *c, entityClass *e)
			{
				c->transform = e->value<transformComponent>(transformComponent::component);
				if (e->has(transformComponent::componentHistory))
					c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
				else
					c->transformHistory = c->transform;
			}

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
				{
					CAGE_LOG_DEBUG(severityEnum::Warning, "engine", "skipping graphics emit (write)");
					return;
				}

				emitWrite = emitBuffers[lock.index()];
				emitWrite->renderableObjects.clear();
				emitWrite->renderableTexts.clear();
				emitWrite->lights.clear();
				emitWrite->cameras.clear();
				emitWrite->emitArena.flush();
				emitWrite->time = time;

				// emit renderable objects
				for (entityClass *e : renderComponent::component->entities())
				{
					emitRenderObjectStruct *c = emitWrite->emitArena.createObject<emitRenderObjectStruct>();
					emitTransform(c, e);
					c->render = e->value<renderComponent>(renderComponent::component);
					if (e->has(animatedTextureComponent::component))
						c->animatedTexture = emitWrite->emitArena.createObject<animatedTextureComponent>(e->value<animatedTextureComponent>(animatedTextureComponent::component));
					if (e->has(animatedSkeletonComponent::component))
						c->animatedSkeleton = emitWrite->emitArena.createObject<animatedSkeletonComponent>(e->value<animatedSkeletonComponent>(animatedSkeletonComponent::component));
					emitWrite->renderableObjects.push_back(c);
				}

				// emit renderable texts
				for (entityClass *e : renderTextComponent::component->entities())
				{
					emitRenderTextStruct *c = emitWrite->emitArena.createObject<emitRenderTextStruct>();
					emitTransform(c, e);
					c->renderText = e->value<renderTextComponent>(renderTextComponent::component);
					emitWrite->renderableTexts.push_back(c);
				}

				// emit lights
				for (entityClass *e : lightComponent::component->entities())
				{
					emitLightStruct *c = emitWrite->emitArena.createObject<emitLightStruct>();
					emitTransform(c, e);
					c->light = e->value<lightComponent>(lightComponent::component);
					if (e->has(shadowmapComponent::component))
						c->shadowmap = emitWrite->emitArena.createObject<shadowmapImpl>(e->value<shadowmapComponent>(shadowmapComponent::component));
					c->entityId = ((uintPtr)e) ^ e->name();
					emitWrite->lights.push_back(c);
				}

				// emit cameras
				for (entityClass *e : cameraComponent::component->entities())
				{
					emitCameraStruct *c = emitWrite->emitArena.createObject<emitCameraStruct>();
					emitTransform(c, e);
					c->camera = e->value<cameraComponent>(cameraComponent::component);
					c->entityId = ((uintPtr)e) ^ e->name();
					emitWrite->cameras.push_back(c);
				}

				emitWrite = nullptr;
			}

			void tick(uint64 time)
			{
				auto lock = swapController->read();
				if (!lock)
				{
					CAGE_LOG_DEBUG(severityEnum::Warning, "engine", "skipping graphics emit (read)");
					return;
				}

				assetManagerClass *ass = assets();
				if (!ass->ready(hashString("cage/cage.pack")))
					return;

				if (!graphicsDispatch->shaderBlit)
				{
					graphicsDispatch->meshSquare = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
					graphicsDispatch->meshSphere = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/sphere.obj"));
					graphicsDispatch->meshCone = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/cone.obj"));
					graphicsDispatch->meshFake = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/fake.obj"));
					graphicsDispatch->shaderVisualizeColor = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/visualize/color.glsl"));
					graphicsDispatch->shaderVisualizeDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/visualize/depth.glsl"));
					graphicsDispatch->shaderVisualizeVelocity = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/visualize/velocity.glsl"));
					graphicsDispatch->shaderBlit = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/blit.glsl"));
					graphicsDispatch->shaderDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/depth.glsl"));
					graphicsDispatch->shaderGBuffer = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/gBuffer.glsl"));
					graphicsDispatch->shaderLighting = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/lighting.glsl"));
					graphicsDispatch->shaderTranslucent = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/translucent.glsl"));
					graphicsDispatch->shaderSsaoGenerate = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/ssaoGenerate.glsl"));
					graphicsDispatch->shaderSsaoBlur = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/ssaoBlur.glsl"));
					graphicsDispatch->shaderSsaoApply = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/ssaoApply.glsl"));
					graphicsDispatch->shaderMotionBlur = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/motionBlur.glsl"));
					graphicsDispatch->shaderLuminanceCollection = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/luminanceCollection.glsl"));
					graphicsDispatch->shaderLuminanceCopy = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/luminanceCopy.glsl"));
					graphicsDispatch->shaderFinalScreen = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/finalScreen.glsl"));
					graphicsDispatch->shaderFxaa = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/effects/fxaa.glsl"));
					graphicsDispatch->shaderFont = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/gui/font.glsl"));
				}

				emitRead = emitBuffers[lock.index()];

				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, controlThread().timePerTick);
				elapsedDispatchTime = dispatchTime - lastDispatchTime;
				lastDispatchTime = dispatchTime;
				shm2d = shmCube = 0;

				graphicsDispatch->firstRenderPass = graphicsDispatch->lastRenderPass = nullptr;
				dispatchArena.flush();

				pointStruct resolution = window()->resolution();
				graphicsDispatch->windowWidth = numeric_cast<uint32>(resolution.x);
				graphicsDispatch->windowHeight = numeric_cast<uint32>(resolution.y);

				if (graphicsDispatch->windowWidth == 0 || graphicsDispatch->windowHeight == 0)
				{
					emitRead = nullptr;
					return;
				}

				{ // update model matrices
					real interFactor = clamp(real(dispatchTime - emitTime) / controlThread().timePerTick, 0, 1);
					for (auto it : emitRead->renderableObjects)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->renderableTexts)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->lights)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->cameras)
						it->updateModelMatrix(interFactor);
					//CAGE_LOG_DEBUG(severityEnum::Info, "engine", string() + "emit: " + emitTime + "\t dispatch time: " + dispatchTime + "\t interpolation: " + interFactor + "\t correction: " + itc.correction);
				}

				// generate shadowmap render passes
				for (auto it : emitRead->lights)
				{
					if (!it->shadowmap)
						continue;
					initializeRenderPassForShadowmap(newRenderPass(), it);
				}

				{ // generate camera render passes
					std::sort(emitRead->cameras.begin(), emitRead->cameras.end(), [](const emitCameraStruct *a, const emitCameraStruct *b)
					{
						CAGE_ASSERT_RUNTIME(a->camera.cameraOrder != b->camera.cameraOrder, a->camera.cameraOrder);
						return a->camera.cameraOrder < b->camera.cameraOrder;
					});
					for (auto it : emitRead->cameras)
					{
						if (graphicsPrepareThread().stereoMode == stereoModeEnum::Mono || it->camera.target)
						{ // mono
							initializeRenderPassForCamera(newRenderPass(), it, eyeEnum::Mono);
						}
						else
						{ // stereo
							initializeRenderPassForCamera(newRenderPass(), it, eyeEnum::Left);
							initializeRenderPassForCamera(newRenderPass(), it, eyeEnum::Right);
						}
					}
				}

				emitRead = nullptr;
			}
		};

		graphicsPrepareImpl *graphicsPrepare;
	}

	mat3x4::mat3x4(const mat4 &in)
	{
		CAGE_ASSERT_RUNTIME(in[3] == 0 && in[7] == 0 && in[11] == 0 && in[15] == 1, in);
		for (uint32 a = 0; a < 4; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 4 + b];
	}

	mat3x4::mat3x4(const mat3 &in)
	{
		for (uint32 a = 0; a < 3; a++)
			for (uint32 b = 0; b < 3; b++)
				data[b][a] = in[a * 3 + b];
	}

	shaderConfigStruct::shaderConfigStruct()
	{
		detail::memset(this, 0, sizeof(shaderConfigStruct));
	}

	objectsStruct::objectsStruct(meshClass *mesh, uint32 max) : shaderMeshes(nullptr), shaderArmatures(nullptr), mesh(mesh), next(nullptr), count(0), max(max)
	{
		assetManagerClass *ass = assets();
		shaderMeshes = (shaderMeshStruct*)graphicsPrepare->dispatchArena.allocate(sizeof(shaderMeshStruct) * max, sizeof(uintPtr));
		if ((mesh->getFlags() & meshFlags::Bones) == meshFlags::Bones)
		{
			CAGE_ASSERT_RUNTIME(mesh->getSkeletonName() == 0 || ass->ready(mesh->getSkeletonName()));
			shaderArmatures = (mat3x4*)graphicsPrepare->dispatchArena.allocate(sizeof(mat3x4) * mesh->getSkeletonBones() * max, sizeof(uintPtr));
		}
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			uint32 n = mesh->textureName(i);
			textures[i] = n ? ass->get<assetSchemeIndexTexture, textureClass>(n) : nullptr;
		}
		graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_VERTEX_SHADER, CAGE_SHADER_ROUTINEUNIF_SKELETON, (mesh->getFlags() & meshFlags::Bones) == meshFlags::Bones ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_ALBEDO])
		{
			if (textures[CAGE_SHADER_TEXTURE_ALBEDO]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY);
			else
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D);
		}
		else
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_SPECIAL])
		{
			if (textures[CAGE_SHADER_TEXTURE_SPECIAL]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY);
			else
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D);
		}
		else
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_NORMAL])
		{
			if (textures[CAGE_SHADER_TEXTURE_NORMAL]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY);
			else
				graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D);
		}
		else
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
	}

	lightsStruct::lightsStruct(lightTypeEnum lightType, sint32 shadowmap, uint32 max) : shaderLights(nullptr), next(nullptr), count(0), max(max), shadowmap(shadowmap), lightType(lightType)
	{
		shaderLights = (shaderLightStruct*)graphicsPrepare->dispatchArena.allocate(sizeof(shaderLightStruct) * max, alignof(shaderLightStruct));
		switch (lightType)
		{
		case lightTypeEnum::Directional:
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW);
			break;
		case lightTypeEnum::Spot:
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOT : CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW);
			break;
		case lightTypeEnum::Point:
			graphicsPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINT : CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW);
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid light type");
		}
	}

	translucentStruct::translucentStruct(meshClass *mesh) : object(mesh, 1), firstLight(nullptr), lastLight(nullptr), next(nullptr)
	{}

	textsStruct::renderStruct::renderStruct() : glyphs(nullptr), count(0), next(nullptr)
	{}

	textsStruct::textsStruct() : firtsRender(nullptr), lastRender(nullptr), font(nullptr), next(nullptr)
	{}

	renderPassStruct::renderPassStruct()
	{
		detail::memset(this, 0, sizeof(renderPassStruct));
	}

	void graphicsPrepareCreate(const engineCreateConfig &config)
	{
		graphicsPrepare = detail::systemArena().createObject<graphicsPrepareImpl>(config);
	}

	void graphicsPrepareDestroy()
	{
		detail::systemArena().destroy<graphicsPrepareImpl>(graphicsPrepare);
		graphicsPrepare = nullptr;
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
