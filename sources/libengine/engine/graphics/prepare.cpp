#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/memory.h>
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
#include <vector>
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
			sint32 index;

			ShadowmapImpl(const ShadowmapComponent &other) : ShadowmapComponent(other), index(0) {}
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
			TextureAnimationComponent *animatedTexture;
			SkeletalAnimationComponent *animatedSkeleton;

			EmitRender()
			{
				detail::memset(this, 0, sizeof(EmitRender));
			}
		};

		struct EmitText : public EmitTransforms
		{
			TextComponent renderText;

			EmitText()
			{
				detail::memset(this, 0, sizeof(EmitText));
			}
		};

		struct EmitLight : public EmitTransforms
		{
			LightComponent light;
			ShadowmapImpl *shadowmap;
			uintPtr entityId;

			EmitLight()
			{
				detail::memset(this, 0, sizeof(EmitLight));
			}
		};

		struct EmitCamera : public EmitTransforms
		{
			CameraComponent camera;
			uintPtr entityId;

			EmitCamera()
			{
				detail::memset(this, 0, sizeof(EmitCamera));
			}
		};

		struct RenderPassImpl : public RenderPass
		{
			mat4 viewProjPrev;
			uint32 sceneMask;
			real lodSelection; // vertical size of screen, at distance of one world-space-unit from camera, in pixels
			bool lodOrthographic;

			RenderPassImpl()
			{
				detail::memset(this, 0, sizeof(RenderPassImpl));
			}
		};

		struct Emit
		{
			MemoryArenaGrowing<MemoryAllocatorPolicyLinear<>, MemoryConcurrentPolicyNone> emitMemory;
			MemoryArena emitArena;

			std::vector<EmitRender*> renderableObjects;
			std::vector<EmitText*> renderableTexts;
			std::vector<EmitLight*> lights;
			std::vector<EmitCamera*> cameras;

			uint64 time;
			bool fresh;

			explicit Emit(const EngineCreateConfig &config) : emitMemory(config.graphicsEmitMemory), emitArena(&emitMemory), time(0), fresh(false)
			{
				renderableObjects.reserve(256);
				renderableTexts.reserve(64);
				lights.reserve(32);
				cameras.reserve(4);
			}

			~Emit()
			{
				emitArena.flush();
			}
		};

		struct GraphicsPrepareImpl
		{
			Emit emitBufferA, emitBufferB, emitBufferC; // this is awfully stupid, damn you c++
			Emit *emitBuffers[3];
			Emit *emitRead, *emitWrite;
			Holder<SwapBufferGuard> swapController;

			mat4 tmpArmature[CAGE_SHADER_MAX_BONES];
			mat4 tmpArmature2[CAGE_SHADER_MAX_BONES];

			MemoryArenaGrowing<MemoryAllocatorPolicyLinear<>, MemoryConcurrentPolicyNone> dispatchMemory;
			MemoryArena dispatchArena;

			InterpolationTimingCorrector itc;
			uint64 emitTime;
			uint64 dispatchTime;
			uint64 lastDispatchTime;
			uint64 elapsedDispatchTime;
			sint32 shm2d, shmCube;

			typedef std::unordered_map<Mesh*, Objects*> OpaqueObjectsMap;
			OpaqueObjectsMap opaqueObjectsMap;
			typedef std::unordered_map<Font*, Texts*> TextsMap;
			TextsMap textsMap;

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
				RenderPassImpl *t = dispatchArena.createObject<RenderPassImpl>();
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

			static void sortTranslucentBackToFront(RenderPassImpl *pass)
			{
				if (!pass->firstTranslucent)
					return;
				OPTICK_EVENT("sort translucent");

				struct Sorter
				{
					const vec3 center;

					uint32 count(Translucent *p)
					{
						uint32 result = 0;
						while (p)
						{
							result++;
							p = p->next;
						}
						return result;
					}

					bool isSorted(Translucent *p)
					{
						if (!p->next)
							return true;
						if (distance(p) < distance(p->next))
							return false;
						return isSorted(p->next);
					}

					explicit Sorter(RenderPassImpl *pass) : center(pass->shaderViewport.eyePos)
					{
						uint32 initialCount = count(pass->firstTranslucent);
						pass->firstTranslucent = sort(pass->firstTranslucent, pass->lastTranslucent);
						pass->lastTranslucent = getTail(pass->firstTranslucent);
						uint32 finalCount = count(pass->firstTranslucent);
						CAGE_ASSERT(initialCount == finalCount, initialCount, finalCount);
						CAGE_ASSERT(isSorted(pass->firstTranslucent));
					}

					vec3 position(Translucent *p)
					{
						const vec4 *m = p->object.shaderMeshes[0].mMat.data;
						return vec3(m[0][3], m[1][3], m[2][3]);
					}

					real distance(Translucent *p)
					{
						return distanceSquared(center, position(p));
					}

					Translucent *partition(Translucent *head, Translucent *end, Translucent *&newHead, Translucent *&newEnd)
					{
						Translucent *prev = nullptr, *cur = head, *pivot = end, *tail = end;
						while (cur != pivot)
						{
							if (distance(cur) > distance(pivot)) // back-to-front
							{
								if (!newHead)
									newHead = cur;
								prev = cur;
								cur = cur->next;
							}
							else
							{
								if (prev)
									prev->next = cur->next;
								Translucent *tmp = cur->next;
								cur->next = nullptr;
								tail->next = cur;
								tail = cur;
								cur = tmp;
							}
						}
						if (!newHead)
							newHead = pivot;
						newEnd = tail;
						return pivot;
					}

					Translucent *getTail(Translucent *cur)
					{
						if (!cur)
							return nullptr;
						while (cur->next)
							cur = cur->next;
						return cur;
					}

					Translucent *sort(Translucent *head, Translucent *end)
					{
						// https://www.geeksforgeeks.org/quicksort-on-singly-linked-list/
						// modified
						if (!head || head == end)
							return head;
						Translucent *newHead = nullptr, *newEnd = nullptr;
						Translucent *pivot = partition(head, end, newHead, newEnd);
						if (newHead != pivot)
						{
							Translucent *tmp = newHead;
							while (tmp->next != pivot)
								tmp = tmp->next;
							tmp->next = nullptr;
							newHead = sort(newHead, tmp);
							tmp = getTail(newHead);
							tmp->next = pivot;
						}
						pivot->next = sort(pivot->next, newEnd);
						return newHead;
					}
				} sorterInstance(pass);
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
				pass->shaderViewport.vpInv = inverse(pass->viewProj);
				pass->shaderViewport.eyePos = camera->model * vec4(0, 0, 0, 1);
				pass->shaderViewport.eyeDir = camera->model * vec4(0, 0, -1, 0);
				pass->shaderViewport.ambientLight = vec4(camera->camera.ambientLight, 0);
				pass->shaderViewport.ambientDirectionalLight = vec4(camera->camera.ambientDirectionalLight, 0);
				pass->shaderViewport.viewport = vec4(pass->vpX, pass->vpY, pass->vpW, pass->vpH);
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
				for (EmitRender *e : emitRead->renderableObjects)
				{
					if ((e->render.sceneMask & pass->sceneMask) == 0 || e->render.object == 0)
						continue;
					CAGE_ASSERT(engineAssets()->ready(e->render.object));
					uint32 schemeIndex = engineAssets()->scheme(e->render.object);
					switch (schemeIndex)
					{
					case assetSchemeIndexMesh:
					{
						Mesh *m = engineAssets()->get<assetSchemeIndexMesh, Mesh>(e->render.object);
						addRenderableMesh(pass, e, m);
					} break;
					case assetSchemeIndexRenderObject:
					{
						RenderObject *o = engineAssets()->get<assetSchemeIndexRenderObject, RenderObject>(e->render.object);
						if (o->lodsCount() == 0)
							continue;
						uint32 lod = 0;
						if (o->lodsCount() > 1)
						{
							real d = 1;
							if (!pass->lodOrthographic)
							{
								vec4 ep4 = e->model * vec4(0, 0, 0, 1);
								d = distance(vec3(ep4) / ep4[3], vec3(pass->shaderViewport.eyePos));
							}
							real f = pass->lodSelection * o->worldSize / (d * o->pixelsSize);
							lod = o->lodSelect(f.value);
						}
						for (uint32 msh : o->meshes(lod))
						{
							Mesh *m = engineAssets()->get<assetSchemeIndexMesh, Mesh>(msh);
							addRenderableMesh(pass, e, m);
						}
					} break;
					default:
						CAGE_LOG(SeverityEnum::Warning, "engine", stringizer() + "trying to render an asset " + e->render.object + " with unsupported scheme " + schemeIndex);
					}
				}
			}

			void addRenderableSkeleton(RenderPassImpl *pass, EmitRender *e, SkeletonRig *s, const mat4 &model, const mat4 &mvp)
			{
				uint32 bonesCount = s->bonesCount();
				if (e->animatedSkeleton && engineAssets()->ready(e->animatedSkeleton->name))
				{
					CAGE_ASSERT(s->bonesCount() == bonesCount, s->bonesCount(), bonesCount);
					const auto &ba = *e->animatedSkeleton;
					SkeletalAnimation *an = engineAssets()->get<assetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ba.name);
					real c = detail::evalCoefficientForSkeletalAnimation(an, dispatchTime, ba.startTime, ba.speed, ba.offset);
					s->animateSkeleton(an, c, tmpArmature, tmpArmature2);
				}
				else
				{
					for (uint32 i = 0; i < bonesCount; i++)
						tmpArmature2[i] = mat4();
				}
				Mesh *mesh = engineAssets()->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/bone.obj"));
				CAGE_ASSERT(mesh->getSkeletonName() == 0);
				for (uint32 i = 0; i < bonesCount; i++)
				{
					e->render.color = colorHsvToRgb(vec3(real(i) / real(bonesCount), 1, 1));
					mat4 m = model * tmpArmature2[i];
					mat4 mvp = pass->viewProj * m;
					addRenderableMesh(pass, e, mesh, m, mvp, mvp);
				}
			}

			void addRenderableMesh(RenderPassImpl *pass, EmitRender *e, Mesh *m)
			{
				addRenderableMesh(pass, e, m, e->model, pass->viewProj * e->model, pass->viewProjPrev * e->modelPrev);
			}

			void addRenderableMesh(RenderPassImpl *pass, EmitRender *e, Mesh *m, const mat4 &model, const mat4 &mvp, const mat4 &mvpPrev)
			{
				if (!frustumCulling(m->getBoundingBox(), mvp))
					return;
				if (pass->targetShadowmap != 0 && none(m->getFlags() & MeshRenderFlags::ShadowCast))
					return;
				if (m->getSkeletonName() && confRenderSkeletonBones)
				{
					SkeletonRig *s = engineAssets()->get<assetSchemeIndexSkeletonRig, SkeletonRig>(m->getSkeletonName());
					addRenderableSkeleton(pass, e, s, model, mvp);
					return;
				}
				Objects *obj = nullptr;
				if (any(m->getFlags() & MeshRenderFlags::Translucency) || e->render.opacity < 1)
				{ // translucent
					Translucent *t = dispatchArena.createObject<Translucent>(m);
					obj = &t->object;
					// add at end
					if (pass->lastTranslucent)
						pass->lastTranslucent->next = t;
					else
						pass->firstTranslucent = t;
					pass->lastTranslucent = t;
					if (any(m->getFlags() & MeshRenderFlags::Lighting))
					{
						for (auto it : emitRead->lights)
							addLight(t, mvp, it); // todo pass other parameters needed for the intersection tests
					}
				}
				else
				{ // opaque
					auto it = opaqueObjectsMap.find(m);
					if (it == opaqueObjectsMap.end())
					{
						uint32 mm = CAGE_SHADER_MAX_INSTANCES;
						mm = min(mm, m->getInstancesLimitHint());
						if (m->getSkeletonBones())
							mm = min(mm, CAGE_SHADER_MAX_BONES / m->getSkeletonBones());
						obj = dispatchArena.createObject<Objects>(m, mm);
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
						CAGE_ASSERT(obj->count < obj->max);
						if (obj->count + 1 == obj->max)
							opaqueObjectsMap.erase(m);
					}
				}
				Objects::ShaderMesh *sm = obj->shaderMeshes + obj->count;
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
					sm->aniTexFrames = detail::evalSamplesForTextureAnimation(obj->textures[CAGE_SHADER_TEXTURE_ALBEDO], dispatchTime, e->animatedTexture->startTime, e->animatedTexture->speed, e->animatedTexture->offset);
				if (obj->shaderArmatures)
				{
					uint32 bonesCount = m->getSkeletonBones();
					Mat3x4 *sa = obj->shaderArmatures + obj->count * bonesCount;
					CAGE_ASSERT(!e->animatedSkeleton || e->animatedSkeleton->name);
					if (e->animatedSkeleton && m->getSkeletonName() && engineAssets()->ready(e->animatedSkeleton->name))
					{
						SkeletonRig *skel = engineAssets()->get<assetSchemeIndexSkeletonRig, SkeletonRig>(m->getSkeletonName());
						CAGE_ASSERT(skel->bonesCount() == bonesCount, skel->bonesCount(), bonesCount);
						const auto &ba = *e->animatedSkeleton;
						SkeletalAnimation *an = engineAssets()->get<assetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ba.name);
						real c = detail::evalCoefficientForSkeletalAnimation(an, dispatchTime, ba.startTime, ba.speed, ba.offset);
						skel->animateSkin(an, c, tmpArmature, tmpArmature2);
						for (uint32 i = 0; i < bonesCount; i++)
							sa[i] = Mat3x4(tmpArmature2[i]);
					}
					else
					{
						for (uint32 i = 0; i < bonesCount; i++)
							sa[i] = Mat3x4();
					}
				}
				obj->count++;
			}

			void addRenderableTexts(RenderPassImpl *pass)
			{
				OPTICK_EVENT("texts");
				for (EmitText *e : emitRead->renderableTexts)
				{
					if ((e->renderText.sceneMask & pass->sceneMask) == 0)
						continue;
					if (!e->renderText.font)
						e->renderText.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
					if (!engineAssets()->ready(e->renderText.font))
						continue;
					string s = loadInternationalizedText(engineAssets(), e->renderText.assetName, e->renderText.textName, e->renderText.value);
					if (s.empty())
						continue;

					Font *font = engineAssets()->get<assetSchemeIndexFont, Font>(e->renderText.font);
					Texts::Render *r = dispatchArena.createObject<Texts::Render>();
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

					Texts *tex = nullptr;
					{
						auto it = textsMap.find(font);
						if (it == textsMap.end())
						{
							tex = dispatchArena.createObject<Texts>();
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

			void addRenderableLights(RenderPassImpl *pass)
			{
				OPTICK_EVENT("lights");
				for (auto it : emitRead->lights)
					addLight(pass, it);
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
					if (!frustumCulling(graphicsDispatch->meshCone->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				case LightTypeEnum::Point:
					mvpMat = pass->viewProj * light->model * mat4::scale(lightRange(light->light.color, light->light.attenuation));
					if (!frustumCulling(graphicsDispatch->meshSphere->getBoundingBox(), mvpMat))
						return; // this light's volume is outside view frustum
					break;
				}
				addLight(pass->firstLight, pass->lastLight, mvpMat, light);
			}

			void addLight(Translucent *trans, const mat4 &mvpMat, EmitLight *light)
			{
				// todo test if the mesh is in range of the light
				addLight(trans->firstLight, trans->lastLight, mvpMat, light);
			}

			void addLight(Lights *&firstLight, Lights *&lastLight, const mat4 &mvpMat, EmitLight *light)
			{
				Lights *lig = nullptr;
				if (light->shadowmap)
				{
					lig = dispatchArena.createObject<Lights>(light->light.lightType, light->shadowmap->index, 1);
					// add at end
					if (lastLight)
						lastLight->next = lig;
					else
						firstLight = lig;
					lastLight = lig;
					lig->shaderLights[0].shadowMat = light->shadowmap->shadowMat;
				}
				else
				{
					for (Lights *it = firstLight; it; it = it->next)
					{
						if (it->count == it->max)
							continue;
						if (it->lightType != light->light.lightType)
							continue;
						if (it->shadowmap != 0)
							continue;
						lig = it;
					}
					if (!lig)
					{
						lig = dispatchArena.createObject<Lights>(light->light.lightType, 0, CAGE_SHADER_MAX_INSTANCES);
						// add at begin
						if (firstLight)
							lig->next = firstLight;
						else
							lastLight = lig;
						firstLight = lig;
					}
				}
				Lights::ShaderLight *sl = lig->shaderLights + lig->count;
				// todo this struct could be precomputed
				sl->mvpMat = mvpMat;
				sl->color = vec4(light->light.color, cos(light->light.spotAngle * 0.5));
				sl->attenuation = vec4(light->light.attenuation, light->light.spotExponent);
				sl->direction = vec4(normalize(vec3(light->model * vec4(0, 0, -1, 0))), 0);
				sl->position = light->model * vec4(0, 0, 0, 1);
				lig->count++;
			}

			GraphicsPrepareImpl(const EngineCreateConfig &config) : emitBufferA(config), emitBufferB(config), emitBufferC(config), emitBuffers{ &emitBufferA, &emitBufferB, &emitBufferC }, emitRead(nullptr), emitWrite(nullptr), dispatchMemory(config.graphicsDispatchMemory), dispatchArena(&dispatchMemory), emitTime(0), dispatchTime(0), lastDispatchTime(0), elapsedDispatchTime(0)
			{
				SwapBufferGuardCreateConfig cfg(3);
				cfg.repeatedReads = true;
				swapController = newSwapBufferGuard(cfg);
			}

			~GraphicsPrepareImpl()
			{
				dispatchArena.flush();
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
				emitWrite->emitArena.flush();
				emitWrite->time = time;
				emitWrite->fresh = true;

				// emit renderable objects
				for (Entity *e : RenderComponent::component->entities())
				{
					EmitRender *c = emitWrite->emitArena.createObject<EmitRender>();
					emitTransform(c, e);
					c->render = e->value<RenderComponent>(RenderComponent::component);
					if (e->has(TextureAnimationComponent::component))
						c->animatedTexture = emitWrite->emitArena.createObject<TextureAnimationComponent>(e->value<TextureAnimationComponent>(TextureAnimationComponent::component));
					if (e->has(SkeletalAnimationComponent::component))
						c->animatedSkeleton = emitWrite->emitArena.createObject<SkeletalAnimationComponent>(e->value<SkeletalAnimationComponent>(SkeletalAnimationComponent::component));
					emitWrite->renderableObjects.push_back(c);
				}

				// emit renderable texts
				for (Entity *e : TextComponent::component->entities())
				{
					EmitText *c = emitWrite->emitArena.createObject<EmitText>();
					emitTransform(c, e);
					c->renderText = e->value<TextComponent>(TextComponent::component);
					emitWrite->renderableTexts.push_back(c);
				}

				// emit lights
				for (Entity *e : LightComponent::component->entities())
				{
					EmitLight *c = emitWrite->emitArena.createObject<EmitLight>();
					emitTransform(c, e);
					c->history.scale = c->current.scale = 1;
					c->light = e->value<LightComponent>(LightComponent::component);
					if (e->has(ShadowmapComponent::component))
						c->shadowmap = emitWrite->emitArena.createObject<ShadowmapImpl>(e->value<ShadowmapComponent>(ShadowmapComponent::component));
					c->entityId = ((uintPtr)e) ^ e->name();
					emitWrite->lights.push_back(c);
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
					EmitCamera *c = emitWrite->emitArena.createObject<EmitCamera>();
					emitTransform(c, e);
					c->history.scale = c->current.scale = 1;
					c->camera = e->value<CameraComponent>(CameraComponent::component);
					c->camera.effects &= effectsMask;
					c->entityId = ((uintPtr)e) ^ e->name();
					emitWrite->cameras.push_back(c);
				}
			}

			void updateDefaultValues(EmitRender *e)
			{
				if (!e->render.object)
					return;

				if (!engineAssets()->ready(e->render.object))
				{
					if (!confRenderMissingMeshes)
					{
						e->render.object = 0; // disable rendering further in the pipeline
						return;
					}
					e->render.object = HashString("cage/mesh/fake.obj");
					CAGE_ASSERT(engineAssets()->ready(e->render.object));
				}

				if (engineAssets()->scheme(e->render.object) == assetSchemeIndexRenderObject)
				{
					RenderObject *o = engineAssets()->get<assetSchemeIndexRenderObject, RenderObject>(e->render.object);

					if (!e->render.color.valid())
						e->render.color = o->color;
					if (!e->render.opacity.valid())
						e->render.opacity = o->opacity;

					{
						TextureAnimationComponent *&c = e->animatedTexture;
						if (!c && (o->texAnimSpeed.valid() || o->texAnimOffset.valid()))
							c = emitRead->emitArena.createObject<TextureAnimationComponent>();
						if (c)
						{
							if (!c->speed.valid())
								c->speed = o->texAnimSpeed;
							if (!c->offset.valid())
								c->offset = o->texAnimOffset;
						}
					}

					{
						SkeletalAnimationComponent *&c = e->animatedSkeleton;
						if (!c && o->skelAnimName)
							c = emitRead->emitArena.createObject<SkeletalAnimationComponent>();
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
					TextureAnimationComponent *c = e->animatedTexture;
					if (!c->speed.valid())
						c->speed = 1;
					if (!c->offset.valid())
						c->offset = 0;
				}

				if (e->animatedSkeleton)
				{
					if (e->animatedSkeleton->name)
					{
						SkeletalAnimationComponent *c = e->animatedSkeleton;
						if (!c->speed.valid())
							c->speed = 1;
						if (!c->offset.valid())
							c->offset = 0;
					}
					else
						e->animatedSkeleton = nullptr;
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
				if (!ass->ready(HashString("cage/cage.pack")) || !ass->ready(HashString("cage/shader/engine/engine.pack")))
					return;

				if (!graphicsDispatch->shaderBlit)
				{
					graphicsDispatch->meshSquare = ass->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/square.obj"));
					graphicsDispatch->meshSphere = ass->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/sphere.obj"));
					graphicsDispatch->meshCone = ass->get<assetSchemeIndexMesh, Mesh>(HashString("cage/mesh/cone.obj"));
					graphicsDispatch->shaderVisualizeColor = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/color.glsl"));
					graphicsDispatch->shaderVisualizeDepth = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/depth.glsl"));
					graphicsDispatch->shaderVisualizeMonochromatic = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/monochromatic.glsl"));
					graphicsDispatch->shaderVisualizeVelocity = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/velocity.glsl"));
					graphicsDispatch->shaderAmbient = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/ambient.glsl"));
					graphicsDispatch->shaderBlit = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
					graphicsDispatch->shaderDepth = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/depth.glsl"));
					graphicsDispatch->shaderGBuffer = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/gBuffer.glsl"));
					graphicsDispatch->shaderLighting = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/lighting.glsl"));
					graphicsDispatch->shaderTranslucent = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/translucent.glsl"));
					graphicsDispatch->shaderGaussianBlur = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/gaussianBlur.glsl"));
					graphicsDispatch->shaderSsaoGenerate = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoGenerate.glsl"));
					graphicsDispatch->shaderSsaoApply = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/ssaoApply.glsl"));
					graphicsDispatch->shaderMotionBlur = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/motionBlur.glsl"));
					graphicsDispatch->shaderBloomGenerate = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomGenerate.glsl"));
					graphicsDispatch->shaderBloomApply = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/bloomApply.glsl"));
					graphicsDispatch->shaderLuminanceCollection = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCollection.glsl"));
					graphicsDispatch->shaderLuminanceCopy = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/luminanceCopy.glsl"));
					graphicsDispatch->shaderFinalScreen = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/finalScreen.glsl"));
					graphicsDispatch->shaderFxaa = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/effects/fxaa.glsl"));
					graphicsDispatch->shaderFont = ass->get<assetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
				}

				emitRead = emitBuffers[lock.index()];
				ClearOnScopeExit resetEmitRead(emitRead);

				emitTime = emitRead->time;
				dispatchTime = itc(emitTime, time, controlThread().updatePeriod());
				elapsedDispatchTime = dispatchTime - lastDispatchTime;
				lastDispatchTime = dispatchTime;
				shm2d = shmCube = 0;

				graphicsDispatch->firstRenderPass = graphicsDispatch->lastRenderPass = nullptr;
				dispatchArena.flush();

				ivec2 resolution = engineWindow()->resolution();
				graphicsDispatch->windowWidth = numeric_cast<uint32>(resolution.x);
				graphicsDispatch->windowHeight = numeric_cast<uint32>(resolution.y);

				if (graphicsDispatch->windowWidth == 0 || graphicsDispatch->windowHeight == 0)
					return;

				if (emitRead->fresh)
				{ // update default values
					OPTICK_EVENT("update default values");
					for (auto it : emitRead->renderableObjects)
						updateDefaultValues(it);
					emitRead->fresh = false;
				}

				{ // update model matrices
					OPTICK_EVENT("update model matrices");
					real interFactor = clamp(real(dispatchTime - emitTime) / controlThread().updatePeriod(), 0, 1);
					for (auto it : emitRead->renderableObjects)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->renderableTexts)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->lights)
						it->updateModelMatrix(interFactor);
					for (auto it : emitRead->cameras)
						it->updateModelMatrix(interFactor);
				}

				// generate shadowmap render passes
				for (auto it : emitRead->lights)
				{
					if (!it->shadowmap)
						continue;
					initializeRenderPassForShadowmap(newRenderPass(), it);
				}

				{ // generate camera render passes
					std::sort(emitRead->cameras.begin(), emitRead->cameras.end(), [](const EmitCamera *a, const EmitCamera *b)
					{
						CAGE_ASSERT(a->camera.cameraOrder != b->camera.cameraOrder, a->camera.cameraOrder);
						return a->camera.cameraOrder < b->camera.cameraOrder;
					});
					for (auto it : emitRead->cameras)
					{
						if (graphicsPrepareThread().stereoMode == StereoModeEnum::Mono || it->camera.target)
						{ // mono
							initializeRenderPassForCamera(newRenderPass(), it, StereoEyeEnum::Mono);
						}
						else
						{ // stereo
							initializeRenderPassForCamera(newRenderPass(), it, StereoEyeEnum::Left);
							initializeRenderPassForCamera(newRenderPass(), it, StereoEyeEnum::Right);
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

	ShaderConfig::ShaderConfig()
	{
		for (uint32 i = 0; i < CAGE_SHADER_MAX_ROUTINES; i++)
			shaderRoutines[i] = m;
	}

	void ShaderConfig::set(uint32 name, uint32 value)
	{
		CAGE_ASSERT(name < CAGE_SHADER_MAX_ROUTINES);
		shaderRoutines[name] = value;
	}

	Objects::Objects(Mesh *mesh, uint32 max) : shaderMeshes(nullptr), shaderArmatures(nullptr), mesh(mesh), next(nullptr), count(0), max(max)
	{
		AssetManager *ass = engineAssets();
		shaderMeshes = (ShaderMesh*)graphicsPrepare->dispatchArena.allocate(sizeof(ShaderMesh) * max, sizeof(uintPtr));
		if (mesh->getSkeletonBones())
		{
			CAGE_ASSERT(mesh->getSkeletonName() == 0 || ass->ready(mesh->getSkeletonName()));
			shaderArmatures = (Mat3x4*)graphicsPrepare->dispatchArena.allocate(sizeof(Mat3x4) * mesh->getSkeletonBones() * max, sizeof(uintPtr));
		}
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			uint32 n = mesh->getTextureName(i);
			textures[i] = n ? ass->get<assetSchemeIndexTexture, Texture>(n) : nullptr;
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

	Lights::Lights(LightTypeEnum lightType, sint32 shadowmap, uint32 max) : shaderLights(nullptr), next(nullptr), count(0), max(max), shadowmap(shadowmap), lightType(lightType)
	{
		shaderLights = (ShaderLight*)graphicsPrepare->dispatchArena.allocate(sizeof(ShaderLight) * max, alignof(ShaderLight));
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

	Translucent::Translucent(Mesh *mesh) : object(mesh, 1), firstLight(nullptr), lastLight(nullptr), next(nullptr)
	{}

	Texts::Render::Render() : glyphs(nullptr), count(0), next(nullptr)
	{}

	Texts::Texts() : firtsRender(nullptr), lastRender(nullptr), font(nullptr), next(nullptr)
	{}

	RenderPass::RenderPass()
	{
		detail::memset(this, 0, sizeof(RenderPass));
	}

	void graphicsPrepareCreate(const EngineCreateConfig &config)
	{
		graphicsPrepare = detail::systemArena().createObject<GraphicsPrepareImpl>(config);
	}

	void graphicsPrepareDestroy()
	{
		detail::systemArena().destroy<GraphicsPrepareImpl>(graphicsPrepare);
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
