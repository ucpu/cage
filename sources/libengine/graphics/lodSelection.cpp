#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-engine/lodSelection.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>

namespace cage
{
	LodSelection::LodSelection(Vec3 center, const CameraComponent &cam, sint32 screenHeightPx) : center(center)
	{
		switch (cam.cameraType)
		{
			case CameraTypeEnum::Orthographic:
			{
				screenSize = cam.orthographicSize[1] * screenHeightPx;
				orthographic = true;
				break;
			}
			case CameraTypeEnum::Perspective:
				screenSize = tan(cam.perspectiveFov * 0.5) * 2 * screenHeightPx;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid camera type");
		}
	}

	uint32 LodSelection::selectLod(const Vec3 position, const RenderObject *object) const
	{
		CAGE_ASSERT(object->lodsCount() > 0);
		uint32 preferredLod = 0;
		if (object->lodsCount() > 1)
		{
			Real d = 1;
			if (!orthographic)
				d = distance(position, center);
			const Real f = screenSize * object->worldSize / (d * object->pixelsSize);
			preferredLod = object->lodSelect(f);
		}
		return preferredLod;
	}

	void LodSelection::selectModels(std::vector<Holder<Model>> &outModels, const Vec3 position, const RenderObject *object, AssetsOnDemand *assets) const
	{
		CAGE_ASSERT(center.valid());
		CAGE_ASSERT(screenSize > 0);

		const uint32 preferredLod = selectLod(position, object);

		// try load the preferred lod
		bool ok = true;
		const auto &fetch = [&](uint32 lod, bool load)
		{
			outModels.clear();
			ok = true;
			for (uint32 it : object->models(lod))
			{
				if (auto md = assets->get<Model>(it, load))
					outModels.push_back(std::move(md));
				else
					ok = false;
			}
		};
		fetch(preferredLod, true);

		// try acquire one level coarser
		if (!ok && preferredLod + 1 < object->lodsCount())
			fetch(preferredLod + 1, false);

		// try acquire one level finer
		if (!ok && preferredLod > 0)
			fetch(preferredLod - 1, false);
	}

	void LodSelection::selectModels(std::vector<Holder<Model>> &outModels, const Vec3 position, const RenderObject *object, const AssetsManager *assets) const
	{
		CAGE_ASSERT(center.valid());
		CAGE_ASSERT(screenSize > 0);

		const uint32 preferredLod = selectLod(position, object);

		// try load the preferred lod
		bool ok = true;
		const auto &fetch = [&](uint32 lod)
		{
			outModels.clear();
			ok = true;
			for (uint32 it : object->models(lod))
			{
				if (auto md = assets->get<Model>(it))
					outModels.push_back(std::move(md));
				else
					ok = false;
			}
		};
		fetch(preferredLod);

		// try acquire one level coarser
		if (!ok && preferredLod + 1 < object->lodsCount())
			fetch(preferredLod + 1);

		// try acquire one level finer
		if (!ok && preferredLod > 0)
			fetch(preferredLod - 1);
	}
}
