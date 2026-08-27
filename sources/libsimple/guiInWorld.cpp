#include <cage-core/assetsManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/entities.h>
#include <cage-core/geometry.h>
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/profiling.h>
#include <cage-core/serialization.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/model.h>
#include <cage-engine/scene.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>
#include <cage-simple/cameraRay.h>
#include <cage-simple/engine.h>
#include <cage-simple/guiInWorld.h>

namespace cage
{
	namespace
	{
		uint32 finishIndex = 0;

		class GuiInWorldImpl : public GuiInWorld
		{
		public:
			GuiInWorldImpl(const GuiInWorldCreateConfig &config) : config(config)
			{
				{
					GuiManagerCreateConfig cfg;
					cfg.assetManager = engineAssets();
					// todo sound
					cfg.tooltipsEnabled = config.tooltipsEnabled;
					guiMan = newGuiManager(cfg);
					guiMan->outputResolution(config.resolution);
					guiMan->outputRetina(config.retinaScale);
				}

				{
					const AssetLabel texLabel = Stringizer() + "gui-in-world-texture-" + (uintPtr)this;
					Holder<Texture> tex = newTexture(engineGraphicsDevice(), ColorTextureCreateConfig{ .resolution = Vec3i(config.resolution[0], config.resolution[1], 1), .renderable = true }, texLabel);
					textureId = engineAssets()->generateUniqueId();
					engineAssets()->loadValue<AssetSchemeIndexTexture>(textureId, tex.share(), texLabel);

					Holder<Mesh> msh = newMesh();
					const Real h = Real(config.resolution[1]) / Real(config.resolution[0]);
					msh->addVertex(Vec3(-1, -h, 0), Vec3(0, 0, -1), Vec2(1, 1));
					msh->addVertex(Vec3(-1, +h, 0), Vec3(0, 0, -1), Vec2(1, 0));
					msh->addVertex(Vec3(+1, +h, 0), Vec3(0, 0, -1), Vec2(0, 0));
					msh->addVertex(Vec3(+1, -h, 0), Vec3(0, 0, -1), Vec2(0, 1));
					msh->indices({ { 0, 1, 2, 0, 2, 3 } });
					MeshImportMaterial material;
					material.specialBase[0] = 1;
					material.specialBase[2] = 1;

					const AssetLabel mshLabel = Stringizer() + "gui-in-world-model-" + (uintPtr)this;
					Holder<Model> mod = newModel(engineGraphicsDevice(), +msh, bufferView(material), mshLabel);
					mod->textureIds[0] = textureId;
					mod->renderFlags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::CutOut;
					modelName = engineAssets()->generateUniqueId();
					engineAssets()->loadValue<AssetSchemeIndexModel>(modelName, mod.share(), mshLabel);
				}
			}

			~GuiInWorldImpl() { cleanUp(); }

			void cleanUp()
			{
				{
					ScopeLock l(mut);
					renderQueue.clear();
				}
				guiMan.clear();
				if (textureId)
					engineAssets()->unload(textureId);
				if (modelName)
					engineAssets()->unload(modelName);
				textureId = modelName = 0;
			}

			struct Intersection
			{
				Vec3 wp = Vec3::Nan(); // world position
				Vec2 gp; // gui position in 0*0 .. w*h
				bool intersects = false;
			};

			Intersection detect(const Line &ray) const
			{
				Intersection res;
				if (!ray.valid())
					return res;
				const Transform &tr = transform;
				const Plane pl = Plane(tr.position, tr.orientation * Vec3(0, 0, -1));
				res.wp = cage::intersection(pl, ray);
				const Vec3 c = res.wp - tr.position;
				const Vec3 r = tr.orientation * Vec3(-1, 0, 0); // the rendered texture is horizontally swapped (see texture coordinates on the render mesh)
				const Vec3 u = tr.orientation * Vec3(0, -1, 0); // the gui has Y axis going top-down, the world has Y going bottom-up
				const Vec2 n = Vec2(dot(c, r), dot(c, u)) / tr.scale * Vec2(1, Real(config.resolution[0]) / Real(config.resolution[1])) * 0.5 + 0.5; // 0*0 .. 1*1
				res.gp = n * Vec2(config.resolution); // 0*0 .. w*h
				res.intersects = c.valid() && n[0] >= 0 && n[0] <= 1 && n[1] >= 0 && n[1] <= 1;
				if (!res.intersects)
					res.wp = Vec3::Nan();
				return res;
			}

			void update(const Line &ray, bool interact)
			{
				ProfilingScope profiling("gui in world update");
				guiMan->prepare();
				Intersection i;
				if (ray.valid())
					i = detect(ray);
				if (i.intersects)
				{
					if (interact)
					{
						guiMan->handleInput(input::MousePress{ { .position = i.gp, .buttons = MouseButtonsFlags::Left } });
						guiMan->handleInput(input::MouseRelease{ { .position = i.gp, .buttons = MouseButtonsFlags::Left } });
					}
					else
						guiMan->handleInput(input::MouseMove{ { .position = i.gp } });
				}
				else
				{
					guiMan->defocus();
					guiMan->handleInput(input::MouseMove{ { .position = Vec2(-1) } });
				}
				auto q = guiMan->finish();
				{
					ScopeLock l(mut);
					renderQueue = std::move(q);
				}
			}

			void dispatchEntry()
			{
				ProfilingScope profiling("gui in world graphics dispatch");

				Holder<GuiRender> qq;
				{
					ScopeLock l(mut);
					qq = std::move(renderQueue); // update the texture once is suficient
				}
				if (!qq)
					return;

				if (!textureId)
					return;
				Holder<Texture> tex = engineAssets()->get<AssetSchemeIndexTexture, Texture>(textureId);
				if (!tex)
					return;

				Holder<GraphicsEncoder> enc = newGraphicsEncoder(engineGraphicsDevice(), "gui-in-world");
				Holder<GraphicsAggregateBuffer> agg = newGraphicsAggregateBuffer({ engineGraphicsDevice() });
				RenderPassConfig passcfg;
				passcfg.colorTargets.push_back({ +tex });
				enc->nextPass(passcfg);
				{
					const auto scope = enc->namedScope("gui-in-world");
					qq->draw({ config.resolution, engineGraphicsDevice(), +enc, +agg });
				}
				agg->submit();
				enc->submit();
			}

			void update()
			{
				if (config.cameraEntity)
				{
					const Line ray = cameraMouseRay(config.cameraEntity);
					update(ray, any(engineWindow()->mouseButtons() & MouseButtonsFlags::Left));
				}
			}

			const GuiInWorldCreateConfig config;
			Holder<Mutex> mut = newMutex();
			Holder<GuiRender> renderQueue; // protected by mutex
			Holder<GuiManager> guiMan;
			Transform transform;
			uint32 textureId = 0;
			uint32 modelName = 0;

			const EventListener<bool()> graphicsDispatchListener = graphicsThread().graphics.listen([this]() { return this->dispatchEntry(); });
			const EventListener<bool()> updateListener = controlThread().update.listen([this]() { return this->update(); });
			const EventListener<bool()> finishListener = controlThread().finalize.listen([this]() { return this->cleanUp(); }, 1'123'321'130 + finishIndex++);
		};
	}

	uint32 GuiInWorld::renderModel() const
	{
		const GuiInWorldImpl *impl = (const GuiInWorldImpl *)this;
		return impl->modelName;
	}

	void GuiInWorld::update(const Transform &tr, bool redraw)
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		impl->transform = tr;
		if (redraw)
			impl->update(Line(), false);
	}

	void GuiInWorld::update(const Line &ray, bool interact)
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		impl->update(ray, interact);
	}

	bool GuiInWorld::intersects(const Line &ray) const
	{
		const GuiInWorldImpl *impl = (const GuiInWorldImpl *)this;
		return impl->detect(ray).intersects;
	}

	Vec3 GuiInWorld::intersection(const Line &ray) const
	{
		const GuiInWorldImpl *impl = (const GuiInWorldImpl *)this;
		return impl->detect(ray).wp;
	}

	GuiManager *GuiInWorld::guiManager()
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		return +impl->guiMan;
	}

	EntityManager *GuiInWorld::guiEntities()
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		return impl->guiMan->entities();
	}

	Holder<GuiInWorld> newGuiInWorld(const GuiInWorldCreateConfig &config)
	{
		return systemMemory().createImpl<GuiInWorld, GuiInWorldImpl>(config);
	}
}
