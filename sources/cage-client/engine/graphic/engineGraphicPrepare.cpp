#include <algorithm>
#include <vector>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/utility/hashString.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/assets.h>
#include <cage-client/engine.h>
#include <cage-client/opengl.h>
#include <cage-client/graphic/shaderConventions.h>

#include "../engine.h"
#include "graphic.h"

namespace cage
{
	namespace
	{
		CAGE_ASSERT_COMPILE(CAGE_SHADER_MAX_ARMATURE_MATRICES == MaxBonesCount, incompatible_shader_definition_with_code);

		configBool renderMissingMeshes("cage-client.engine.debugRenderMissingMeshes", false);

		struct shadowmapImpl : public shadowmapComponent
		{
			mat4 shadowMat;
			sint32 index;
			shadowmapImpl(const shadowmapComponent &other) : shadowmapComponent(other), index(0) {}
		};

		struct configuredSkeletonImpl
		{
			configuredSkeletonComponent current;
			configuredSkeletonComponent history;
			configuredSkeletonImpl(entityClass *e)
			{
				current = e->value<configuredSkeletonComponent>(configuredSkeletonComponent::component);
				if (e->hasComponent(configuredSkeletonComponent::componentHistory))
					history = e->value<configuredSkeletonComponent>(configuredSkeletonComponent::componentHistory);
				else
					history = current;
			}
		};

		struct emitTransformsStruct
		{
			transformComponent transform, transformHistory;
			mat4 model;
			void updateModelMatrix(real interFactor)
			{
				model = mat4(interpolate(transformHistory, transform, interFactor));
			}
		};

		struct emitRenderStruct : public emitTransformsStruct
		{
			renderComponent render;
			animatedTextureComponent *animatedTexture;
			animatedSkeletonComponent *animatedSkeleton;
			configuredSkeletonImpl *configuredSkeleton;
			emitRenderStruct()
			{
				detail::memset(this, 0, sizeof(emitRenderStruct));
			}
		};

		struct emitLightStruct : public emitTransformsStruct
		{
			lightComponent light;
			shadowmapImpl *shadowmap;
			emitLightStruct()
			{
				detail::memset(this, 0, sizeof(emitLightStruct));
			}
		};

		struct emitCameraStruct : public emitTransformsStruct
		{
			cameraComponent camera;
			emitCameraStruct()
			{
				detail::memset(this, 0, sizeof(emitCameraStruct));
			}
		};

		struct renderPassImpl : public renderPassStruct
		{
			mat4 view;
			mat4 proj;
			mat4 viewProj;
			uint32 renderMask;
		};

		struct graphicPrepareImpl
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> emitMemory;
			memoryArena emitArena;
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> dispatchMemory;
			memoryArena dispatchArena;

			std::vector<emitRenderStruct*> emitRenderables;
			std::vector<emitLightStruct*> emitLights;
			std::vector<emitCameraStruct*> emitCameras;

			sint32 shm2d, shmCube;
			uint64 controlTime;
			uint64 prepareTime;

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
				renderPassImpl *t = dispatchArena.createObject<renderPassImpl>();
				if (graphicDispatch->firstRenderPass)
					graphicDispatch->lastRenderPass->next = t;
				else
					graphicDispatch->firstRenderPass = t;
				graphicDispatch->lastRenderPass = t;
				return t;
			}

			void initializeRenderPassForCamera(renderPassImpl *pass, emitCameraStruct *camera, eyeEnum eye)
			{
				mat4 mat = camera->model;
				if (camera->camera.target)
				{
					uint32 w = 0, h = 0;
					camera->camera.target->getResolution(w, h);
					if (w == 0 || h == 0)
						CAGE_THROW_ERROR(exception, "target texture has zero resolution");
					switch (camera->camera.cameraType)
					{
					case cameraTypeEnum::Orthographic:
						pass->proj = orthographicProjection(-camera->camera.orthographicSize[0], camera->camera.orthographicSize[0], -camera->camera.orthographicSize[1], camera->camera.orthographicSize[1], camera->camera.near, camera->camera.far);
						break;
					case cameraTypeEnum::Perspective:
						pass->proj = perspectiveProjection(camera->camera.perspectiveFov, real(w) / real(h), camera->camera.near, camera->camera.far);
						break;
					default:
						CAGE_THROW_ERROR(exception, "invalid camera type");
					}
					vec3 p = vec3(mat * vec4(0, 0, 0, 1));
					pass->view = lookAt(p, p + vec3(mat * vec4(0, 0, -1, 0)), vec3(mat * vec4(0, 1, 0, 0)));
					pass->vpX = numeric_cast<uint32>(camera->camera.viewportOrigin[0] * w);
					pass->vpY = numeric_cast<uint32>(camera->camera.viewportOrigin[1] * h);
					pass->vpW = numeric_cast<uint32>(camera->camera.viewportSize[0] * w);
					pass->vpH = numeric_cast<uint32>(camera->camera.viewportSize[1] * h);
				}
				else
				{
					real x = camera->camera.viewportOrigin[0], y = camera->camera.viewportOrigin[1], w = camera->camera.viewportSize[0], h = camera->camera.viewportSize[1];
					stereoCameraStruct cam;
					cam.direction = vec3(mat * vec4(0, 0, -1, 0));
					cam.position = vec3(mat * vec4(0, 0, 0, 1));
					cam.worldUp = vec3(mat * vec4(0, 1, 0, 0));
					cam.fov = camera->camera.perspectiveFov;
					cam.near = camera->camera.near;
					cam.far = camera->camera.far;
					cam.zeroParallaxDistance = camera->camera.zeroParallaxDistance;
					cam.eyeSeparation = camera->camera.eyeSeparation;
					cam.ortographic = camera->camera.cameraType == cameraTypeEnum::Orthographic;
					stereoscopy(eye, cam, real(graphicDispatch->windowWidth) / real(graphicDispatch->windowHeight), (stereoModeEnum)graphicPrepareThread::stereoMode, pass->view, pass->proj, x, y, w, h);
					if (camera->camera.cameraType == cameraTypeEnum::Orthographic)
						pass->proj = orthographicProjection(-camera->camera.orthographicSize[0], camera->camera.orthographicSize[0], -camera->camera.orthographicSize[1], camera->camera.orthographicSize[1], camera->camera.near, camera->camera.far);
					pass->vpX = numeric_cast<uint32>(x * real(graphicDispatch->windowWidth));
					pass->vpY = numeric_cast<uint32>(y * real(graphicDispatch->windowHeight));
					pass->vpW = numeric_cast<uint32>(w * real(graphicDispatch->windowWidth));
					pass->vpH = numeric_cast<uint32>(h * real(graphicDispatch->windowHeight));
				}
				pass->viewProj = pass->proj * pass->view;
				pass->shaderViewport.vpInv = pass->viewProj.inverse();
				pass->shaderViewport.eyePos = mat * vec4(0, 0, 0, 1);
				pass->shaderViewport.ambientLight = vec4(camera->camera.ambientLight, 0);
				pass->shaderViewport.viewport = vec4(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
				pass->targetTexture = camera->camera.target;
				pass->clearFlags = ((camera->camera.clear & cameraClearFlags::Color) == cameraClearFlags::Color ? GL_COLOR_BUFFER_BIT : 0) | ((camera->camera.clear & cameraClearFlags::Depth) == cameraClearFlags::Depth ? GL_DEPTH_BUFFER_BIT : 0);
				pass->renderMask = camera->camera.renderMask;
				addRenderableObjects(pass, false);
				for (auto it = emitLights.begin(), et = emitLights.end(); it != et; it++)
					addLight(pass, *it);
			}

			void initializeRenderPassForShadowmap(renderPassImpl *pass, emitLightStruct *light)
			{
				pass->view = light->model.inverse();
				switch (light->light.lightType)
				{
				case lightTypeEnum::Directional:
					pass->proj = orthographicProjection(-light->shadowmap->worldRadius[0], light->shadowmap->worldRadius[0], -light->shadowmap->worldRadius[1], light->shadowmap->worldRadius[1], -light->shadowmap->worldRadius[2], light->shadowmap->worldRadius[2]);
					break;
				case lightTypeEnum::Spot:
					pass->proj = perspectiveProjection(light->light.spotAngle, 1, light->shadowmap->worldRadius[0], light->shadowmap->worldRadius[1]);
					break;
				case lightTypeEnum::Point:
					pass->proj = perspectiveProjection(degs(90), 1, light->shadowmap->worldRadius[0], light->shadowmap->worldRadius[1]);
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid light type");
				}
				pass->viewProj = pass->proj * pass->view;
				pass->shadowmapResolution = pass->vpW = pass->vpH = light->shadowmap->resolution;
				pass->renderMask = 0xffffffff;
				pass->targetShadowmap = light->light.lightType == lightTypeEnum::Point ? (-shmCube++ - 1) : (shm2d++ + 1);
				light->shadowmap->index = pass->targetShadowmap;
				static const mat4 bias = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.498, 1.0);
				light->shadowmap->shadowMat = bias * pass->viewProj;
				addRenderableObjects(pass, true);
			}

			void addRenderableObjects(renderPassImpl *pass, bool shadows)
			{
				const vec3 up = vec3(pass->view.inverse() * vec4(0, 1, 0, 0));
				for (auto it = emitRenderables.begin(), et = emitRenderables.end(); it != et; it++)
				{
					emitRenderStruct *e = *it;
					if ((e->render.renderMask & pass->renderMask) == 0)
						continue;
					mat4 mvp = pass->viewProj * e->model;
					if (!assets()->ready(e->render.object))
					{
						if (renderMissingMeshes)
							addRenderableMesh(pass, e, graphicDispatch->meshFake, e->model, mvp);
						continue;
					}
					switch (assets()->scheme(e->render.object))
					{
					case assetSchemeIndexMesh:
					{
						meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(e->render.object);
						addRenderableMesh(pass, e, m, e->model, mvp);
					} break;
					case assetSchemeIndexObject:
					{
						objectClass *o = assets()->get<assetSchemeIndexObject, objectClass>(e->render.object);
						if (o->lodsCount() == 0)
							continue;
						uint32 lod = 0;
						if (shadows)
						{
							if (o->shadower)
							{
								CAGE_ASSERT_RUNTIME(assets()->ready(o->shadower), e->render.object, o->shadower);
								meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(o->shadower);
								addRenderableMesh(pass, e, m, e->model, mvp);
								continue;
							}
							lod = o->lodsCount() - 1;
						}
						else if (o->worldSize > 0 && o->pixelsSize > 0 && o->lodsCount() > 1)
						{
							vec4 a4 = mvp * vec4(0,0,0,1);
							vec4 b4 = mvp * vec4(up * o->worldSize, 1);
							vec2 a2 = vec2(a4) / a4[3];
							vec2 b2 = vec2(b4) / b4[3];
							real f = length(a2 - b2) * pass->vpH / o->pixelsSize;
							for (uint32 i = 0; i < o->lodsCount(); i++)
							{
								if (f < o->lodsThreshold(i))
									lod = i;
							}
						}
						for (uint32 mshi = 0, mshe = o->meshesCount(lod); mshi < mshe; mshi++)
						{
							meshClass *m = assets()->get<assetSchemeIndexMesh, meshClass>(o->meshesName(lod, mshi));
							addRenderableMesh(pass, e, m, e->model, mvp);
						}
					} break;
					default:
						CAGE_THROW_CRITICAL(exception, "render an invalid-scheme asset");
					}
				}
			}

			void addRenderableMesh(renderPassImpl *pass, emitRenderStruct *e, meshClass *m, const mat4 &model, const mat4 &mvp)
			{
				if (!frustumCulling(m->getBoundingBox(), mvp))
					return;
				if (pass->targetShadowmap != 0 && (m->getFlags() & meshFlags::ShadowCast) == meshFlags::None)
					return;
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
					for (objectsStruct *it = pass->firstOpaque; it; it = it->next)
					{
						if (it->count == it->max)
							continue;
						if (it->mesh != m)
							continue;
						obj = it;
						break;
					}
					if (!obj)
					{
						obj = dispatchArena.createObject<objectsStruct>(m, CAGE_SHADER_MAX_RENDER_INSTANCES);
						// add at begin
						if (pass->firstOpaque)
							obj->next = pass->firstOpaque;
						else
							pass->lastOpaque = obj;
						pass->firstOpaque = obj;
					}
				}
				objectsStruct::shaderMeshStruct *sm = obj->shaderMeshes + obj->count;
				sm->color = vec4(e->render.color, 0);
				sm->mMat = model;
				sm->mvpMat = mvp;
				sm->normalMat = mat4(modelToNormal(model));
				sm->normalMat[15] = ((m->getFlags() & meshFlags::Lighting) == meshFlags::Lighting) ? 1 : 0; // is ligting enabled
				if (e->animatedTexture)
					sm->aniTexFrames = detail::evalSamplesForTextureAnimation(obj->textures[CAGE_SHADER_TEXTURE_ALBEDO], prepareTime, e->animatedTexture->animationStart, e->animatedTexture->animationSpeed, e->animatedTexture->animationOffset);
				if (obj->shaderArmatures)
				{
					//objectsStruct::shaderArmatureStruct *sa = obj->shaderArmatures + obj->count;
					// todo
				}
				obj->count++;
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
					if (!frustumCulling(graphicDispatch->meshCone->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				case lightTypeEnum::Point:
					mvpMat = pass->viewProj * e->model * mat4(lightRange(e->light.color, e->light.attenuation));
					if (!frustumCulling(graphicDispatch->meshSphere->getBoundingBox(), mvpMat))
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
						lig = dispatchArena.createObject<lightsStruct>(e->light.lightType, 0, CAGE_SHADER_MAX_RENDER_INSTANCES);
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

			graphicPrepareImpl(const engineCreateConfig &config) : emitMemory(config.graphicEmitMemory), dispatchMemory(config.graphicDispatchMemory)
			{
				emitRenderables.reserve(256);
				emitLights.reserve(32);
				emitCameras.reserve(4);
			}

			void initialize()
			{
				emitArena = memoryArena(&emitMemory);
				dispatchArena = memoryArena(&dispatchMemory);
			}

			void finalize()
			{
				emitArena.flush();
				emitArena = memoryArena();
				dispatchArena.flush();
				dispatchArena = memoryArena();
			}

			void emit()
			{
				emitRenderables.clear();
				emitLights.clear();
				emitCameras.clear();
				emitArena.flush();

				{ // emit renderables
					uint32 cnt = renderComponent::component->getComponentEntities()->entitiesCount();
					entityClass *const *ents = renderComponent::component->getComponentEntities()->entitiesArray();
					for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
					{
						entityClass *e = *i;
						emitRenderStruct *c = emitArena.createObject<emitRenderStruct>();
						c->transform = e->value<transformComponent>(transformComponent::component);
						if (e->hasComponent(transformComponent::componentHistory))
							c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
						else
							c->transformHistory = c->transform;
						c->render = e->value<renderComponent>(renderComponent::component);
						if (e->hasComponent(animatedTextureComponent::component))
							c->animatedTexture = emitArena.createObject<animatedTextureComponent>(e->value<animatedTextureComponent>(animatedTextureComponent::component));
						if (e->hasComponent(animatedSkeletonComponent::component))
							c->animatedSkeleton = emitArena.createObject<animatedSkeletonComponent>(e->value<animatedSkeletonComponent>(animatedSkeletonComponent::component));
						if (e->hasComponent(configuredSkeletonComponent::component))
							c->configuredSkeleton = emitArena.createObject<configuredSkeletonImpl>(e);
						emitRenderables.push_back(c);
					}
				}
				{ // emit lights
					uint32 cnt = lightComponent::component->getComponentEntities()->entitiesCount();
					entityClass *const *ents = lightComponent::component->getComponentEntities()->entitiesArray();
					for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
					{
						entityClass *e = *i;
						emitLightStruct *c = emitArena.createObject<emitLightStruct>();
						c->transform = e->value<transformComponent>(transformComponent::component);
						if (e->hasComponent(transformComponent::componentHistory))
							c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
						else
							c->transformHistory = c->transform;
						c->light = e->value<lightComponent>(lightComponent::component);
						if (e->hasComponent(shadowmapComponent::component))
							c->shadowmap = emitArena.createObject<shadowmapImpl>(e->value<shadowmapComponent>(shadowmapComponent::component));
						emitLights.push_back(c);
					}
				}
				{ // emit cameras
					uint32 cnt = cameraComponent::component->getComponentEntities()->entitiesCount();
					entityClass *const *ents = cameraComponent::component->getComponentEntities()->entitiesArray();
					for (entityClass *const *i = ents, *const *ie = ents + cnt; i != ie; i++)
					{
						entityClass *e = *i;
						emitCameraStruct *c = emitArena.createObject<emitCameraStruct>();
						c->transform = e->value<transformComponent>(transformComponent::component);
						if (e->hasComponent(transformComponent::componentHistory))
							c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
						else
							c->transformHistory = c->transform;
						c->camera = e->value<cameraComponent>(cameraComponent::component);
						emitCameras.push_back(c);
					}
				}
			}

			void tick(uint64 controlTime, uint64 prepareTime)
			{
				assetManagerClass *ass = assets();
				if (!ass->ready(hashString("cage/cage.pack")))
					return;

				this->controlTime = controlTime;
				this->prepareTime = prepareTime;
				shm2d = shmCube = 0;

				if (!graphicDispatch->shaderBlitColor)
				{
					graphicDispatch->meshSquare = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
					graphicDispatch->meshSphere = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/sphere.obj"));
					graphicDispatch->meshCone = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/cone.obj"));
					graphicDispatch->meshFake = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/fake.obj"));
					graphicDispatch->shaderBlitColor = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/blitColor.glsl"));
					graphicDispatch->shaderBlitDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/blitDepth.glsl"));
					graphicDispatch->shaderDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/depth.glsl"));
					graphicDispatch->shaderGBuffer = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/gBuffer.glsl"));
					graphicDispatch->shaderLighting = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/lighting.glsl"));
					graphicDispatch->shaderTranslucent = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/translucent.glsl"));
				}

				graphicDispatch->firstRenderPass = graphicDispatch->lastRenderPass = nullptr;
				dispatchArena.flush();

				pointStruct resolution = window()->resolution();
				graphicDispatch->windowWidth = resolution.x;
				graphicDispatch->windowHeight = resolution.y;

				{ // update model matrices
					real interFactor = clamp(real(prepareTime - controlTime) / controlThread::timePerTick, 0, 1);
					for (auto it = emitLights.begin(), et = emitLights.end(); it != et; it++)
						(*it)->updateModelMatrix(interFactor);
					for (auto it = emitCameras.begin(), ite = emitCameras.end(); it != ite; it++)
						(*it)->updateModelMatrix(interFactor);
					for (auto it = emitRenderables.begin(), et = emitRenderables.end(); it != et; it++)
						(*it)->updateModelMatrix(interFactor);
				}

				// generate shadowmap render passes
				for (auto it = emitLights.begin(), et = emitLights.end(); it != et; it++)
				{
					if (!(*it)->shadowmap)
						continue;
					initializeRenderPassForShadowmap(newRenderPass(), *it);
				}

				{ // generate camera render passes
					struct cameraComparatorStruct
					{
						const bool operator() (const emitCameraStruct *a, const emitCameraStruct *b) const
						{
							if (!!a->camera.target == !!b->camera.target)
								return a->camera.cameraOrder < b->camera.cameraOrder;
							return a->camera.target > b->camera.target;
						}
					};
					std::sort(emitCameras.begin(), emitCameras.end(), cameraComparatorStruct());
					for (auto it = emitCameras.begin(), ite = emitCameras.end(); it != ite; it++)
					{
						if (graphicPrepareThread::stereoMode == stereoModeEnum::Mono || (*it)->camera.target)
						{ // mono
							initializeRenderPassForCamera(newRenderPass(), *it, eyeEnum::Mono);
						}
						else
						{ // stereo
							initializeRenderPassForCamera(newRenderPass(), *it, eyeEnum::Left);
							initializeRenderPassForCamera(newRenderPass(), *it, eyeEnum::Right);
						}
					}
				}
			}
		};

		graphicPrepareImpl *graphicPrepare;
	}

	shaderConfigStruct::shaderConfigStruct()
	{
		detail::memset(this, 0, sizeof(shaderConfigStruct));
	}

	objectsStruct::objectsStruct(meshClass *mesh, uint32 max) : shaderMeshes(nullptr), shaderArmatures(nullptr), mesh(mesh), next(nullptr), count(0), max(max)
	{
		shaderMeshes = (shaderMeshStruct*)graphicPrepare->dispatchArena.allocate(sizeof(shaderMeshStruct) * max);
		if ((mesh->getFlags() & meshFlags::Bones) == meshFlags::Bones)
			shaderArmatures = (shaderArmatureStruct*)graphicPrepare->dispatchArena.allocate(sizeof(shaderArmatureStruct) * max);
		else
			shaderArmatures = nullptr;
		assetManagerClass *ass = assets();
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			uint32 n = mesh->textureName(i);
			textures[i] = n ? ass->get<assetSchemeIndexTexture, textureClass>(n) : nullptr;
		}
		graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_VERTEX_SHADER, CAGE_SHADER_ROUTINEUNIF_SKELETON, (mesh->getFlags() & meshFlags::Bones) == meshFlags::Bones ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_ALBEDO])
		{
			if (textures[CAGE_SHADER_TEXTURE_ALBEDO]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY);
			else
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D);
		}
		else
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPALBEDO, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_SPECIAL])
		{
			if (textures[CAGE_SHADER_TEXTURE_SPECIAL]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY);
			else
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D);
		}
		else
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
		if (textures[CAGE_SHADER_TEXTURE_NORMAL])
		{
			if (textures[CAGE_SHADER_TEXTURE_NORMAL]->getTarget() == GL_TEXTURE_2D_ARRAY)
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY);
			else
				graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D);
		}
		else
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_MAPNORMAL, CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING);
	}

	lightsStruct::lightsStruct(lightTypeEnum lightType, sint32 shadowmap, uint32 max) : shaderLights(nullptr), next(nullptr), count(0), max(max), shadowmap(shadowmap), lightType(lightType)
	{
		shaderLights = (shaderLightStruct*)graphicPrepare->dispatchArena.allocate(sizeof(shaderLightStruct) * max);
		switch (lightType)
		{
		case lightTypeEnum::Directional:
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW);
			break;
		case lightTypeEnum::Spot:
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOT : CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW);
			break;
		case lightTypeEnum::Point:
			graphicPrepareImpl::setShaderRoutine(&shaderConfig, GL_FRAGMENT_SHADER, CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE, shadowmap == 0 ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINT : CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW);
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid light type");
		}
	}

	translucentStruct::translucentStruct(meshClass *mesh) : object(mesh, 1), firstLight(nullptr), lastLight(nullptr), next(nullptr)
	{}

	renderPassStruct::renderPassStruct()
	{
		detail::memset(this, 0, sizeof(renderPassStruct));
	}

	void graphicPrepareCreate(const engineCreateConfig &config)
	{
		graphicPrepare = detail::systemArena().createObject<graphicPrepareImpl>(config);
	}

	void graphicPrepareDestroy()
	{
		detail::systemArena().destroy<graphicPrepareImpl>(graphicPrepare);
		graphicPrepare = nullptr;
	}

	void graphicPrepareInitialize()
	{
		graphicPrepare->initialize();
	}

	void graphicPrepareFinalize()
	{
		graphicPrepare->finalize();
	}

	void graphicPrepareEmit()
	{
		graphicPrepare->emit();
	}

	void graphicPrepareTick(uint64 controlTime, uint64 prepareTime)
	{
		graphicPrepare->tick(controlTime, prepareTime);
	}
}
