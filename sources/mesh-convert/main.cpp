#include <vector>

#include <cage-core/color.h>
#include <cage-core/files.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/meshExport.h>
#include <cage-core/meshImport.h>

using namespace cage;

// these contain paths relative to output
std::vector<String> texsAlbedo, texsAlbedoPremultiplied, texsNormal, texsBump, texsSpecial;
std::vector<std::pair<String, String>> outModels;

bool mergeParts = false;
bool convertToCage = false;
Real scale = 1;

uint32 exportTexture(const MeshImportTexture &tex, const String &path)
{
	for (const auto &part : tex.images.parts)
	{
		if (part.cubeFace != 0 || part.layer != 0 || part.mipmapLevel != 0)
			continue;
		if (part.image->channels() == 2)
			imageConvert(+part.image, 3); // avoid confusing alpha with other types
		part.image->exportFile(path);
		return part.image->channels();
	}
	CAGE_THROW_ERROR(Exception, "exported no image");
}

void convertTexture(const MeshImportTexture &tex, MeshExportGltfConfig &cfg, const String &meshName, const String &output)
{
	switch (tex.type)
	{
		case MeshImportTextureType::Albedo:
		{
			CAGE_ASSERT(cfg.albedo.filename.empty());
			cfg.albedo.filename = meshName + "_albedo.png";
			if (exportTexture(tex, pathJoin(output, cfg.albedo.filename)) > 3)
				texsAlbedoPremultiplied.push_back(cfg.albedo.filename);
			else
				texsAlbedo.push_back(cfg.albedo.filename);
			break;
		}
		case MeshImportTextureType::Normal:
		{
			CAGE_ASSERT(cfg.normal.filename.empty());
			cfg.normal.filename = meshName + "_normal.png";
			exportTexture(tex, pathJoin(output, cfg.normal.filename));
			texsNormal.push_back(cfg.normal.filename);
			break;
		}
		case MeshImportTextureType::Bump:
		{
			CAGE_ASSERT(cfg.normal.filename.empty());
			cfg.normal.filename = meshName + "_bump.png";
			exportTexture(tex, pathJoin(output, cfg.normal.filename));
			texsBump.push_back(cfg.normal.filename);
			break;
		}
		case MeshImportTextureType::Special:
		{
			CAGE_ASSERT(cfg.pbr.filename.empty());
			cfg.pbr.filename = meshName + "_special.png";
			exportTexture(tex, pathJoin(output, cfg.pbr.filename));
			texsSpecial.push_back(cfg.pbr.filename);
			break;
		}
		case MeshImportTextureType::GltfPbr:
		{
			CAGE_ASSERT(cfg.pbr.filename.empty());
			cfg.pbr.filename = meshName + "_pbr.png";
			exportTexture(tex, pathJoin(output, cfg.pbr.filename));
			break;
		}
		default:
		{
			if (convertToCage)
			{
				CAGE_THROW_ERROR(Exception, "unknown type of texture");
			}
			else
			{
				CAGE_LOG(SeverityEnum::Warning, "meshConv", Stringizer() + "unused texture: " + tex.name + ", type: " + meshImportTextureTypeToString(tex.type));
				exportTexture(tex, pathJoin(output, Stringizer() + meshName + "_" + meshImportTextureTypeToString(tex.type) + ".png"));
			}
			break;
		}
	}
}

void convertMaterial(const MeshImportPart &part, const MeshExportGltfConfig &cfg, const String &filePath)
{
	Holder<File> mat = writeFile(filePath);

	mat->writeLine("[base]");
	if (Vec3(part.material.albedoBase) != Vec3())
		mat->writeLine(Stringizer() + "albedo = " + colorLinearToGamma(Vec3(part.material.albedoBase)));
	if (part.material.albedoBase[3] != 0)
		mat->writeLine(Stringizer() + "opacity = " + part.material.albedoBase[3]);
	if (part.material.specialBase[0] != 0)
		mat->writeLine(Stringizer() + "roughness = " + part.material.specialBase[0]);
	if (part.material.specialBase[1] != 0)
		mat->writeLine(Stringizer() + "metallic = " + part.material.specialBase[1]);
	if (part.material.specialBase[2] != 0)
		mat->writeLine(Stringizer() + "emission = " + part.material.specialBase[2]);
	if (part.material.specialBase[3] != 0)
		mat->writeLine(Stringizer() + "mask = " + part.material.specialBase[3]);
	mat->writeLine("");

	mat->writeLine("[mult]");
	if (Vec3(part.material.albedoMult) != Vec3(1))
		mat->writeLine(Stringizer() + "albedo = " + Vec3(part.material.albedoMult));
	if (part.material.albedoMult[3] != 1)
		mat->writeLine(Stringizer() + "opacity = " + part.material.albedoMult[3]);
	if (part.material.specialMult[0] != 1)
		mat->writeLine(Stringizer() + "roughness = " + part.material.specialMult[0]);
	if (part.material.specialMult[1] != 1)
		mat->writeLine(Stringizer() + "metallic = " + part.material.specialMult[1]);
	if (part.material.specialMult[2] != 1)
		mat->writeLine(Stringizer() + "emission = " + part.material.specialMult[2]);
	if (part.material.specialMult[3] != 1)
		mat->writeLine(Stringizer() + "mask = " + part.material.specialMult[3]);
	mat->writeLine("");

	mat->writeLine("[textures]");
	if (!cfg.albedo.filename.empty())
		mat->writeLine(Stringizer() + "albedo = " + cfg.albedo.filename);
	if (!cfg.normal.filename.empty())
		mat->writeLine(Stringizer() + "normal = " + cfg.normal.filename);
	if (!cfg.pbr.filename.empty())
		mat->writeLine(Stringizer() + "special = " + cfg.pbr.filename);
	mat->writeLine("");

	mat->writeLine("[render]");
	if (!part.shaderName.empty())
		mat->writeLine(Stringizer() + "shader = " + part.shaderName);
	if (part.renderLayer != 0)
		mat->writeLine(Stringizer() + "layer = " + part.renderLayer);
	mat->writeLine("");

	mat->writeLine("[flags]");
	if (any(part.renderFlags & MeshRenderFlags::CutOut))
		mat->writeLine("cutOut");
	if (any(part.renderFlags & MeshRenderFlags::Transparent))
		mat->writeLine("transparent");
	if (any(part.renderFlags & MeshRenderFlags::Fade))
		mat->writeLine("fade");
	if (any(part.renderFlags & MeshRenderFlags::TwoSided))
		mat->writeLine("twoSided");
	if (none(part.renderFlags & MeshRenderFlags::DepthTest))
		mat->writeLine("noDepthTest");
	if (none(part.renderFlags & MeshRenderFlags::DepthWrite))
		mat->writeLine("noDepthWrite");
	if (none(part.renderFlags & MeshRenderFlags::Lighting))
		mat->writeLine("noLighting");
	if (none(part.renderFlags & MeshRenderFlags::ShadowCast))
		mat->writeLine("noShadowCast");
	mat->writeLine("");

	mat->close();
}

void convertFile(const String &input, const String &output)
{
	CAGE_LOG(SeverityEnum::Info, "meshConv", Stringizer() + "converting file: " + input);
	MeshImportConfig impConf;
	impConf.discardSkeleton = mergeParts; // this should help merge more parts
	impConf.mergeParts = mergeParts;
	MeshImportResult mr = meshImportFiles(input, impConf);

	if (convertToCage)
		meshImportConvertToCageFormats(mr);
	else
		meshImportNormalizeFormats(mr);

	{ // no bones yet
		if (mr.skeleton || mr.animations.size() > 0)
			CAGE_LOG(SeverityEnum::Warning, "meshConv", "converting skeletal animations is not yet supported");
		for (MeshImportPart &part : mr.parts)
		{
			part.mesh->boneIndices(PointerRange<Vec4i>());
			part.mesh->boneWeights(PointerRange<Vec4>());
		}
	}

	if (scale != 1)
	{
		for (MeshImportPart &part : mr.parts)
			meshApplyTransform(+part.mesh, Transform(Vec3(), Quat(), scale));
	}

	const String baseName = pathExtractFilenameNoExtension(input);
	uint32 partIndex = 0;
	for (const MeshImportPart &part : mr.parts)
	{
		MeshExportGltfConfig cfg;
		cfg.mesh = +part.mesh;
		cfg.renderFlags = part.renderFlags;

		String meshName = baseName;
		if (mr.parts.size() > 1)
			meshName += Stringizer() + "_" + partIndex;

		for (const MeshImportTexture &tex : part.textures)
			convertTexture(tex, cfg, meshName, output);

		String matName;
		if (convertToCage)
		{
			matName = meshName + ".cpm";
			convertMaterial(part, cfg, pathJoin(output, matName));
		}

		meshName += ".glb";
		meshExportFiles(pathJoin(output, meshName), cfg);
		outModels.push_back({ meshName, matName });
		partIndex++;
	}
}

void generateConfiguration(const String &output)
{
	const String name = pathExtractFilename(output);
	Holder<File> f = writeFile(pathJoin(output, Stringizer() + name + ".assets"));

	for (const auto &model : outModels)
	{
		f->writeLine("[]");
		f->writeLine("scheme = model");
		if (!model.second.empty())
			f->writeLine(Stringizer() + "material = " + model.second);
		f->writeLine(model.first);
		f->writeLine("");
	}

	if (!texsAlbedoPremultiplied.empty())
	{
		f->writeLine("[]");
		f->writeLine("scheme = texture");
		f->writeLine("srgb = true");
		f->writeLine("premultiplyAlpha = true");
		for (const String &it : texsAlbedoPremultiplied)
			f->writeLine(it);
		f->writeLine("");
	}

	if (!texsAlbedo.empty())
	{
		f->writeLine("[]");
		f->writeLine("scheme = texture");
		f->writeLine("srgb = true");
		for (const String &it : texsAlbedo)
			f->writeLine(it);
		f->writeLine("");
	}

	if (!texsNormal.empty())
	{
		f->writeLine("[]");
		f->writeLine("scheme = texture");
		f->writeLine("normal = true");
		for (const String &it : texsNormal)
			f->writeLine(it);
		f->writeLine("");
	}

	if (!texsBump.empty())
	{
		f->writeLine("[]");
		f->writeLine("scheme = texture");
		f->writeLine("convert = heightToNormal");
		f->writeLine("normal = true");
		for (const String &it : texsBump)
			f->writeLine(it);
		f->writeLine("");
	}

	if (!texsSpecial.empty())
	{
		f->writeLine("[]");
		f->writeLine("scheme = texture");
		for (const String &it : texsSpecial)
			f->writeLine(it);
		f->writeLine("");
	}

	if (outModels.size() > 1)
	{
		const String objName = Stringizer() + name + ".object";

		{
			Holder<File> o = writeFile(pathJoin(output, objName));
			o->writeLine("[]");
			for (const auto &model : outModels)
				o->writeLine(model.first);
			o->close();
		}

		f->writeLine("[]");
		f->writeLine("scheme = object");
		f->writeLine(objName);
		f->writeLine("");
	}

	f->close();
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
		const String outPath = cmd->cmdString('o', "output", "converted-meshes");
		mergeParts = cmd->cmdBool('m', "merge", mergeParts);
		convertToCage = cmd->cmdBool('c', "cage", convertToCage);
		scale = cmd->cmdFloat('s', "scale", scale.value);
		const auto &inPaths = cmd->cmdArray(0, "--");
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -o output -- mesh1.fbx mesh2.fbx");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (inPaths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		if (pathIsFile(outPath))
			CAGE_THROW_ERROR(Exception, "output already exists");
		for (const String &path : inPaths)
			convertFile(path, outPath);
		if (convertToCage)
			generateConfiguration(outPath);
		CAGE_LOG(SeverityEnum::Info, "meshConv", "all done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
