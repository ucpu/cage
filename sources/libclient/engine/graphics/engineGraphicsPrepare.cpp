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
#include <cage-core/utility/swapBufferController.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/engine.h>
#include <cage-client/opengl.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/utility/stereoscopy.h>
#include <cage-client/window.h>

#include "../engine.h"
#include "graphics.h"

namespace cage
{
	namespace
	{
		CAGE_ASSERT_COMPILE(CAGE_SHADER_MAX_ARMATURE_MATRICES == MaxBonesCount, incompatible_shader_definition_with_code);

		configBool renderMissingMeshes("cage-client.engine.renderMissingMeshes", false);

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

		struct emitStruct
		{
			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> emitMemory;
			memoryArena emitArena;

			std::vector<emitRenderStruct*> renderables;
			std::vector<emitLightStruct*> lights;
			std::vector<emitCameraStruct*> cameras;

			uint64 time;

			emitStruct(const engineCreateConfig &config) : emitMemory(config.graphicsEmitMemory), emitArena(&emitMemory), time(0)
			{
				renderables.reserve(256);
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
			emitStruct emitBuffers[3];
			emitStruct *emitRead, *emitWrite;
			holder<swapBufferControllerClass> swapController;

			mat4 tmpArmature[MaxBonesCount];

			memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> dispatchMemory;
			memoryArena dispatchArena;

			interpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			sint32 shm2d, shmCube;

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
				if (graphicsDispatch->firstRenderPass)
					graphicsDispatch->lastRenderPass->next = t;
				else
					graphicsDispatch->firstRenderPass = t;
				graphicsDispatch->lastRenderPass = t;
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
					stereoscopy(eye, cam, real(graphicsDispatch->windowWidth) / real(graphicsDispatch->windowHeight), (stereoModeEnum)graphicsPrepareThread().stereoMode, pass->view, pass->proj, x, y, w, h);
					if (camera->camera.cameraType == cameraTypeEnum::Orthographic)
						pass->proj = orthographicProjection(-camera->camera.orthographicSize[0], camera->camera.orthographicSize[0], -camera->camera.orthographicSize[1], camera->camera.orthographicSize[1], camera->camera.near, camera->camera.far);
					pass->vpX = numeric_cast<uint32>(x * real(graphicsDispatch->windowWidth));
					pass->vpY = numeric_cast<uint32>(y * real(graphicsDispatch->windowHeight));
					pass->vpW = numeric_cast<uint32>(w * real(graphicsDispatch->windowWidth));
					pass->vpH = numeric_cast<uint32>(h * real(graphicsDispatch->windowHeight));
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
				for (auto it : emitRead->lights)
					addLight(pass, it);
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
				for (emitRenderStruct *e : emitRead->renderables)
				{
					if ((e->render.renderMask & pass->renderMask) == 0)
						continue;
					mat4 mvp = pass->viewProj * e->model;
					if (!assets()->ready(e->render.object))
					{
						if (renderMissingMeshes)
							addRenderableMesh(pass, e, graphicsDispatch->meshFake, e->model, mvp);
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
					sm->aniTexFrames = detail::evalSamplesForTextureAnimation(obj->textures[CAGE_SHADER_TEXTURE_ALBEDO], dispatchTime, e->animatedTexture->startTime, e->animatedTexture->speed, e->animatedTexture->offset);
				if (obj->shaderArmatures)
				{
					objectsStruct::shaderArmatureStruct *sa = obj->shaderArmatures + obj->count;
					uint32 bonesCount = m->getSkeletonBones();
					if (e->configuredSkeleton)
					{
						for (uint32 i = 0; i < bonesCount; i++)
							sa->armature[i] = e->configuredSkeleton->current.configuration[i];
					}
					else if (e->animatedSkeleton && m->getSkeletonName() != 0 && assets()->ready(e->animatedSkeleton->name))
					{
						skeletonClass *skel = assets()->get<assetSchemeIndexSkeleton, skeletonClass>(m->getSkeletonName());
						CAGE_ASSERT_RUNTIME(skel->bonesCount() == bonesCount, skel->bonesCount(), bonesCount);
						const auto &ba = *e->animatedSkeleton;
						animationClass *an = assets()->get<assetSchemeIndexAnimation, animationClass>(ba.name);
						real c = detail::evalCoefficientForSkeletalAnimation(an, dispatchTime, ba.startTime, ba.speed, ba.offset);
						skel->evaluatePose(an, c, tmpArmature, sa->armature);
					}
					else
					{
						for (uint32 i = 0; i < bonesCount; i++)
							sa->armature[i] = mat4();
					}
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

			graphicsPrepareImpl(const engineCreateConfig &config) : emitBuffers{ config, config, config }, emitRead(nullptr), emitWrite(nullptr), dispatchMemory(config.graphicsDispatchMemory), dispatchArena(&dispatchMemory), emitTime(0), dispatchTime(0)
			{
				swapBufferControllerCreateConfig cfg(3);
				cfg.repeatedReads = true;
				swapController = newSwapBufferController(cfg);
			}

			~graphicsPrepareImpl()
			{
				dispatchArena.flush();
			}

			void emit(uint64 time)
			{
				auto lock = swapController->write();
				if (!lock)
				{
					CAGE_LOG_DEBUG(severityEnum::Warning, "engine", "skipping graphics emit (write)");
					return;
				}

				emitWrite = &emitBuffers[lock.index()];
				emitWrite->renderables.clear();
				emitWrite->lights.clear();
				emitWrite->cameras.clear();
				emitWrite->emitArena.flush();
				emitWrite->time = time;

				// emit renderables
				for (entityClass *e : renderComponent::component->getComponentEntities()->entities())
				{
					emitRenderStruct *c = emitWrite->emitArena.createObject<emitRenderStruct>();
					c->transform = e->value<transformComponent>(transformComponent::component);
					if (e->hasComponent(transformComponent::componentHistory))
						c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->render = e->value<renderComponent>(renderComponent::component);
					if (e->hasComponent(animatedTextureComponent::component))
						c->animatedTexture = emitWrite->emitArena.createObject<animatedTextureComponent>(e->value<animatedTextureComponent>(animatedTextureComponent::component));
					if (e->hasComponent(animatedSkeletonComponent::component))
						c->animatedSkeleton = emitWrite->emitArena.createObject<animatedSkeletonComponent>(e->value<animatedSkeletonComponent>(animatedSkeletonComponent::component));
					if (e->hasComponent(configuredSkeletonComponent::component))
						c->configuredSkeleton = emitWrite->emitArena.createObject<configuredSkeletonImpl>(e);
					emitWrite->renderables.push_back(c);
				}

				// emit lights
				for (entityClass *e : lightComponent::component->getComponentEntities()->entities())
				{
					emitLightStruct *c = emitWrite->emitArena.createObject<emitLightStruct>();
					c->transform = e->value<transformComponent>(transformComponent::component);
					if (e->hasComponent(transformComponent::componentHistory))
						c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->light = e->value<lightComponent>(lightComponent::component);
					if (e->hasComponent(shadowmapComponent::component))
						c->shadowmap = emitWrite->emitArena.createObject<shadowmapImpl>(e->value<shadowmapComponent>(shadowmapComponent::component));
					emitWrite->lights.push_back(c);
				}

				// emit cameras
				for (entityClass *e : cameraComponent::component->getComponentEntities()->entities())
				{
					emitCameraStruct *c = emitWrite->emitArena.createObject<emitCameraStruct>();
					c->transform = e->value<transformComponent>(transformComponent::component);
					if (e->hasComponent(transformComponent::componentHistory))
						c->transformHistory = e->value<transformComponent>(transformComponent::componentHistory);
					else
						c->transformHistory = c->transform;
					c->camera = e->value<cameraComponent>(cameraComponent::component);
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

				if (!graphicsDispatch->shaderBlitColor)
				{
					graphicsDispatch->meshSquare = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/square.obj"));
					graphicsDispatch->meshSphere = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/sphere.obj"));
					graphicsDispatch->meshCone = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/cone.obj"));
					graphicsDispatch->meshFake = ass->get<assetSchemeIndexMesh, meshClass>(hashString("cage/mesh/fake.obj"));
					graphicsDispatch->shaderBlitColor = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/blitColor.glsl"));
					graphicsDispatch->shaderBlitDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/blitDepth.glsl"));
					graphicsDispatch->shaderDepth = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/depth.glsl"));
					graphicsDispatch->shaderGBuffer = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/gBuffer.glsl"));
					graphicsDispatch->shaderLighting = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/lighting.glsl"));
					graphicsDispatch->shaderTranslucent = ass->get<assetSchemeIndexShader, shaderClass>(hashString("cage/shader/engine/translucent.glsl"));
				}

				emitRead = &emitBuffers[lock.index()];

				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, controlThread().timePerTick);
				shm2d = shmCube = 0;

				graphicsDispatch->firstRenderPass = graphicsDispatch->lastRenderPass = nullptr;
				dispatchArena.flush();

				pointStruct resolution = window()->resolution();
				graphicsDispatch->windowWidth = resolution.x;
				graphicsDispatch->windowHeight = resolution.y;


				{ // update model matrices
					real interFactor = clamp(real(dispatchTime - emitTime) / controlThread().timePerTick, 0, 1);
					for (auto it : emitRead->lights)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->cameras)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->renderables)
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
					struct cameraComparatorStruct
					{
						const bool operator() (const emitCameraStruct *a, const emitCameraStruct *b) const
						{
							if (!!a->camera.target == !!b->camera.target)
								return a->camera.cameraOrder < b->camera.cameraOrder;
							return a->camera.target > b->camera.target;
						}
					};
					std::sort(emitRead->cameras.begin(), emitRead->cameras.end(), cameraComparatorStruct());
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

	shaderConfigStruct::shaderConfigStruct()
	{
		detail::memset(this, 0, sizeof(shaderConfigStruct));
	}

	objectsStruct::objectsStruct(meshClass *mesh, uint32 max) : shaderMeshes(nullptr), shaderArmatures(nullptr), mesh(mesh), next(nullptr), count(0), max(max)
	{
		assetManagerClass *ass = assets();
		shaderMeshes = (shaderMeshStruct*)graphicsPrepare->dispatchArena.allocate(sizeof(shaderMeshStruct) * max);
		if ((mesh->getFlags() & meshFlags::Bones) == meshFlags::Bones)
		{
			CAGE_ASSERT_RUNTIME(mesh->getSkeletonName() == 0 || ass->ready(mesh->getSkeletonName()));
			shaderArmatures = (shaderArmatureStruct*)graphicsPrepare->dispatchArena.allocate(sizeof(shaderArmatureStruct) * max);
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
		shaderLights = (shaderLightStruct*)graphicsPrepare->dispatchArena.allocate(sizeof(shaderLightStruct) * max);
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
