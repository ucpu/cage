#ifndef guard_guiInWorld_sed3f98gz7jik418f
#define guard_guiInWorld_sed3f98gz7jik418f

#include <cage-engine/core.h>

namespace cage
{
	class Entity;
	class GuiManager;
	class EntityManager;

	struct GuiInWorldCreateConfig
	{
		Entity *renderEntity = nullptr; // mandatory
		Entity *cameraEntity = nullptr; // optional
		Vec2i resolution = Vec2i(1920, 1080);
		Real retinaScale = 4;
	};

	class GuiInWorld : private Immovable
	{
	public:
		void update(const Line &ray, bool interact);
		void cleanUp();
		GuiManager *guiManager();
		EntityManager *guiEntities();
	};

	Holder<GuiInWorld> newGuiInWorld(const GuiInWorldCreateConfig &config);
}

#endif // guard_guiInWorld_sed3f98gz7jik418f
