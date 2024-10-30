#include "../image-info/imageInfo.h"

#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>

using namespace cage;

void info(const String &src, int format)
{
	CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "opening file: " + src);
	MeshImportResult msh;
	try
	{
		msh = meshImportFiles(src);
		switch (format)
		{
			case 1:
				CAGE_LOG(SeverityEnum::Info, "mesh", "normalizing format");
				meshImportNormalizeFormats(msh);
				break;
			case 2:
				CAGE_LOG(SeverityEnum::Info, "mesh", "converting to cage format");
				meshImportConvertToCageFormats(msh);
				break;
		}
	}
	catch (const Exception &)
	{
		return;
	}

	CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "contains " + msh.parts.size() + " parts");
	CAGE_LOG(SeverityEnum::Info, "mesh", "");
	Aabb overallBox;
	for (const auto &pt : msh.parts)
	{
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "part name: " + pt.objectName);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "mesh type: " + meshTypeToString(pt.mesh->type()));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "indices count: " + pt.mesh->indicesCount());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "vertices count: " + pt.mesh->verticesCount());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "has uvs: " + !pt.mesh->uvs().empty());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "has uvs 3D: " + !pt.mesh->uvs3().empty());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "has normals: " + !pt.mesh->normals().empty());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "has bones: " + !pt.mesh->boneIndices().empty());
		const Aabb box = pt.mesh->boundingBox();
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "bounding box: " + box);
		overallBox += box;
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "material name: " + pt.materialName);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "albedo base: " + pt.material.albedoBase);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "albedo mult: " + pt.material.albedoMult);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "special base: " + pt.material.specialBase);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "special mult: " + pt.material.specialMult);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "cutOut: " + any(pt.renderFlags & MeshRenderFlags::CutOut));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "transparent: " + any(pt.renderFlags & MeshRenderFlags::Transparent));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "fade: " + any(pt.renderFlags & MeshRenderFlags::Fade));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "two sided: " + any(pt.renderFlags & MeshRenderFlags::TwoSided));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "depth test: " + any(pt.renderFlags & MeshRenderFlags::DepthTest));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "depth write: " + any(pt.renderFlags & MeshRenderFlags::DepthWrite));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "shadow cast: " + any(pt.renderFlags & MeshRenderFlags::ShadowCast));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "lighting: " + any(pt.renderFlags & MeshRenderFlags::Lighting));
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "uses " + pt.textures.size() + " textures");
		CAGE_LOG(SeverityEnum::Info, "mesh", "");
		for (const auto &tx : pt.textures)
		{
			CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "texture name: " + tx.name);
			CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "texture type: " + meshImportTextureTypeToString(tx.type));
			imageInfo(tx.images);
			CAGE_LOG(SeverityEnum::Info, "mesh", "");
		}
	}
	CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "overall bounding box: " + overallBox);
	CAGE_LOG(SeverityEnum::Info, "mesh", "");

	if (msh.skeleton)
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "skeleton bones: " + msh.skeleton->bonesCount());
	else
		CAGE_LOG(SeverityEnum::Info, "mesh", "no skeleton");

	CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "contains " + msh.animations.size() + " animations");
	CAGE_LOG(SeverityEnum::Info, "mesh", "");
	for (const auto &ani : msh.animations)
	{
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "animation name: " + ani.name);
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "bones count: " + ani.animation->bonesCount());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "channels count: " + ani.animation->channelsCount());
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "duration: " + ani.animation->duration() + " us");
		CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "duration: " + (ani.animation->duration() * 1e-6) + " s");
		CAGE_LOG(SeverityEnum::Info, "mesh", "");
	}

	CAGE_LOG(SeverityEnum::Info, "mesh", Stringizer() + "accessed " + msh.paths.size() + " files");
	for (const auto &pth : msh.paths)
		CAGE_LOG(SeverityEnum::Info, "mesh", pth);
	CAGE_LOG(SeverityEnum::Info, "mesh", "");
}

bool logFilter(const detail::LoggerInfo &i)
{
	const String c = String(i.component);
	return c != "assimp" && c != "meshImport";
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger()->filter.bind<logFilter>();
	try
	{
		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		const auto &paths = cmd->cmdArray(0, "--");
		const bool normalizeFormats = cmd->cmdBool('n', "normalize", false);
		const bool convertToCage = cmd->cmdBool('c', "cage", false);
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -c -- mesh.glb");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "--normalize: loads external textures and converts them to appropriate number of channels");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "--cage: convert to cage specific formats");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (normalizeFormats && convertToCage)
			CAGE_THROW_ERROR(Exception, "at most one of --normalize and --cage is allowed");
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		for (const String &path : paths)
			info(path, convertToCage ? 2 : normalizeFormats ? 1 : 0);
		CAGE_LOG(SeverityEnum::Info, "mesh", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
