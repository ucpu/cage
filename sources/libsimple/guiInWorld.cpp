#include <cage-core/assetManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/entities.h>
#include <cage-core/geometry.h>
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/profiling.h>
#include <cage-core/serialization.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderQueue.h>
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
		class GuiInWorldImpl : public GuiInWorld
		{
		public:
			GuiInWorldImpl(const GuiInWorldCreateConfig &config) : config(config)
			{
				CAGE_ASSERT(config.renderEntity);

				{
					GuiManagerCreateConfig cfg;
					cfg.assetManager = engineAssets();
					cfg.provisionalGraphics = +engineProvisonalGraphics();
					cfg.tooltipsEnabled = config.tooltipsEnabled;
					guiMan = newGuiManager(cfg);
					guiMan->outputResolution(config.resolution);
					guiMan->outputRetina(config.retinaScale);
				}
			}

			~GuiInWorldImpl() { cleanUp(); }

			struct Intersection
			{
				Vec3 wp = Vec3::Nan(); // world position
				Vec2 gp; // gui position in 0*0 .. w*h
				bool intersects = false;
			};

			CAGE_FORCE_INLINE Intersection detect(const Line &ray) const
			{
				Intersection res;
				if (!ray.valid())
					return res;
				const Transform tr = config.renderEntity->value<TransformComponent>();
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

				const auto i = detect(ray);
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
					guiMan->focus(0);
					guiMan->handleInput(input::MouseMove{ { .position = Vec2(-1) } });
				}

				auto q = guiMan->finish();
				{
					ScopeLock l(mut);
					renderQueue = std::move(q);
				}
			}

			void cleanUp()
			{
				if (textureName)
					engineAssets()->remove(textureName);
				textureName = 0;
				if (modelName)
					engineAssets()->remove(modelName);
				modelName = 0;
			}

			void dispatchEntry()
			{
				ProfilingScope profiling("gui in world graphics dispatch");

				Holder<Texture> tex;
				if (firstFrame)
				{
					firstFrame = false;

					tex = newTexture();
					tex->initialize(config.resolution, 1, GL_RGBA8);
					tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					tex->filters(GL_LINEAR, GL_LINEAR, 8);
					tex->setDebugName(Stringizer() + "gui-in-world-texture-" + (uintPtr)this);
					textureName = engineAssets()->generateUniqueName();
					engineAssets()->fabricate<AssetSchemeIndexTexture>(textureName, tex.share(), Stringizer() + "gui-in-world-texture-" + (uintPtr)this);

					Holder<Mesh> msh = newMesh();
					const Real h = Real(config.resolution[1]) / Real(config.resolution[0]);
					msh->addVertex(Vec3(-1, -h, 0), Vec3(0, 0, -1), Vec2(1, 0));
					msh->addVertex(Vec3(-1, +h, 0), Vec3(0, 0, -1), Vec2(1, 1));
					msh->addVertex(Vec3(+1, +h, 0), Vec3(0, 0, -1), Vec2(0, 1));
					msh->addVertex(Vec3(+1, -h, 0), Vec3(0, 0, -1), Vec2(0, 0));
					msh->indices({ { 0, 1, 2, 0, 2, 3 } });
					MeshImportMaterial material;
					material.specialBase[0] = 1;
					material.specialBase[2] = 1;

					Holder<Model> mod = newModel();
					mod->importMesh(+msh, bufferView(material));
					mod->textureNames[0] = textureName;
					mod->flags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::CutOut;
					mod->setDebugName(Stringizer() + "gui-in-world-model-" + (uintPtr)this);
					modelName = engineAssets()->generateUniqueName();
					engineAssets()->fabricate<AssetSchemeIndexModel>(modelName, mod.share(), Stringizer() + "gui-in-world-model-" + (uintPtr)this);
				}
				else
				{
					tex = engineAssets()->get<AssetSchemeIndexTexture, Texture>(textureName);
					if (!tex)
						return;
				}

				// retrieve this every frame so that it is not removed
				auto fb = engineProvisonalGraphics()->frameBufferDraw(Stringizer() + "gui-in-world-framebuffer-" + (uintPtr)this);

				Holder<RenderQueue> qq;
				{
					ScopeLock l(mut);
					qq = std::move(renderQueue);
				}
				if (!qq)
					return;

				Holder<RenderQueue> q = newRenderQueue("gui in world", engineProvisonalGraphics());
				{
					auto name = q->namedScope("gui in world");
					q->bind(fb);
					q->colorTexture(fb, 0, tex);
					q->viewport(Vec2i(), config.resolution);
					q->clear(true, false);
					q->enqueue(std::move(qq));
					q->resetAllState();
					q->resetFrameBuffer();
				}
				q->dispatch();
			}

			void update()
			{
				config.renderEntity->value<RenderComponent>().object = modelName;
				if (config.cameraEntity)
				{
					const Line ray = cameraMouseRay(config.cameraEntity);
					update(ray, any(engineWindow()->mouseButtons() & MouseButtonsFlags::Left));
				}
			}

			const GuiInWorldCreateConfig config;
			Holder<Mutex> mut = newMutex();
			Holder<RenderQueue> renderQueue; // protected by mutex
			Holder<GuiManager> guiMan;
			uint32 textureName = 0;
			uint32 modelName = 0;
			bool firstFrame = true;

			const EventListener<bool()> graphicsDispatchListener = graphicsDispatchThread().dispatch.listen([this]() { return this->dispatchEntry(); });
			const EventListener<bool()> updateListener = controlThread().update.listen([this]() { return this->update(); });
		};
	}

	bool GuiInWorld::intersects(const Line &ray) const
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		return impl->detect(ray).intersects;
	}

	Vec3 GuiInWorld::intersection(const Line &ray) const
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		return impl->detect(ray).wp;
	}

	void GuiInWorld::update(const Line &ray, bool interact)
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		impl->update(ray, interact);
	}

	void GuiInWorld::cleanUp()
	{
		GuiInWorldImpl *impl = (GuiInWorldImpl *)this;
		return impl->cleanUp();
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
