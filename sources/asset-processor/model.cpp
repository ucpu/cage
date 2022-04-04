#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/hashString.h>
#include <cage-engine/shaderConventions.h>

#include "processor.h"

#include <set>

MeshImportConfig meshImportConfig(bool allowAxes)
{
	MeshImportConfig config;
	config.rootPath = inputDirectory;
	if (allowAxes)
	{
		config.axesSwizzle = toLower(properties("axes"));
		config.scale = toFloat(properties("scale"));
	}
	config.mergeParts = toBool(properties("bakeModel"));
	config.verbose = true;
	return config;
}

void meshImportNotifyUsedFiles(const MeshImportResult &result)
{
	for (const String &p : result.paths)
		writeLine(cage::String("use = ") + pathToRel(p, inputDirectory));
}

uint32 meshImportSelectIndex(const MeshImportResult &result)
{
	if (result.parts.size() == 1 && inputSpec.empty())
	{
		CAGE_LOG(SeverityEnum::Note, "selectModel", "using the first model, because it is the only model available");
		return 0;
	}

	if (isDigitsOnly(inputSpec) && !inputSpec.empty())
	{
		const uint32 n = toUint32(inputSpec);
		if (n < result.parts.size())
		{
			CAGE_LOG(SeverityEnum::Note, "selectModel", Stringizer() + "using model index " + n + ", because the input specifier is numeric");
			return n;
		}
		else
			CAGE_THROW_ERROR(Exception, "the input specifier is numeric, but the index is out of range");
	}

	std::set<uint32> candidates;
	for (uint32 modelIndex = 0; modelIndex < result.parts.size(); modelIndex++)
	{
		const String objName = result.parts[modelIndex].objectName;
		if (objName == inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "selectModel", Stringizer() + "considering model index " + modelIndex + ", because the model name is matching");
		}
		const String matName = result.parts[modelIndex].materialName;
		if (matName == inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "selectModel", Stringizer() + "considering model index " + modelIndex + ", because the material name matches");
		}
		const String comb = objName + "_" + matName;
		if (comb == inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "selectModel", Stringizer() + "considering model index " + modelIndex + ", because the combined name matches");
		}
	}

	switch (candidates.size())
	{
	case 0:
		CAGE_THROW_ERROR(Exception, "file does not contain requested model");
	case 1:
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "using model at index " + *candidates.begin());
		return *candidates.begin();
	default:
		CAGE_THROW_ERROR(Exception, "requested name is not unique");
	}
}

namespace
{
	enum class ModelDataFlags : uint32
	{
		None = 0,
		Normals = 1 << 0,
		Tangents = 1 << 1,
		Bones = 1 << 2,
		Uvs2 = 1 << 3,
		Uvs3 = 1 << 4,
	};
}

namespace cage
{
	GCHL_ENUM_BITS(ModelDataFlags);
}

namespace
{
	void setFlags(ModelDataFlags &flags, ModelDataFlags idx, bool available, const char *name)
	{
		bool requested = toBool(properties(name));
		if (requested && !available)
		{
			CAGE_LOG_THROW(cage::String("requested feature: ") + name);
			CAGE_THROW_ERROR(Exception, "requested feature not available");
		}
		if (requested)
		{
			flags |= idx;
			CAGE_LOG(SeverityEnum::Info, logComponentName, cage::Stringizer() + "feature requested: " + name);
		}
	}

	void printMaterial(const ModelHeader &dsm, const MeshImportMaterial &mat)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "specialBase: " + mat.specialBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "specialMult: " + mat.specialMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "translucent: " + any(dsm.renderFlags & MeshRenderFlags::Translucent));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "alphaClip: " + any(dsm.renderFlags & MeshRenderFlags::AlphaClip));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "two sides: " + any(dsm.renderFlags & MeshRenderFlags::TwoSided));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "depth test: " + any(dsm.renderFlags & MeshRenderFlags::DepthTest));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "depth write: " + any(dsm.renderFlags & MeshRenderFlags::DepthWrite));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "shadow cast: " + any(dsm.renderFlags & MeshRenderFlags::ShadowCast));
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "lighting: " + any(dsm.renderFlags & MeshRenderFlags::Lighting));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i])
				CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void validateFlags(const ModelHeader &dsm, const ModelDataFlags flags, const MeshRenderFlags renderFlags, const MeshImportMaterial &mat)
	{
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && none(flags & (ModelDataFlags::Uvs2 | ModelDataFlags::Uvs3)))
				CAGE_THROW_ERROR(Exception, "material has a texture and no uvs");
		}

		if (any(renderFlags & MeshRenderFlags::Translucent) && any(renderFlags & MeshRenderFlags::AlphaClip))
			CAGE_THROW_ERROR(Exception, "material has both translucent and alphaClip flags");

		if (any(flags & ModelDataFlags::Tangents) && none(flags & ModelDataFlags::Uvs2))
			CAGE_THROW_ERROR(Exception, "tangents are exported, but uvs are missing");
		if (any(flags & ModelDataFlags::Tangents) && none(flags & ModelDataFlags::Normals))
			CAGE_THROW_ERROR(Exception, "tangents are exported, but normals are missing");

		if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && none(flags & ModelDataFlags::Normals))
			CAGE_THROW_ERROR(Exception, "model uses normal map texture but has no normals");
		if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && none(flags & ModelDataFlags::Tangents))
			CAGE_THROW_ERROR(Exception, "model uses normal map texture but has no tangents");
	}

	String convertReferencePath(const String &input)
	{
		String detail;
		String p = input;
		{
			const uint32 sep = min(find(p, '?'), find(p, ';'));
			detail = subString(p, sep, m);
			p = subString(p, 0, sep);
		}
		p = pathToRel(p, inputDirectory) + detail;
		writeLine(cage::String("ref = ") + p);
		return p;
	}
}

void processModel()
{
	MeshImportConfig config = meshImportConfig(true);
	config.materialPathOverride = properties("material");
	config.materialNameAlternative = inputSpec;
	config.generateNormals = toBool(properties("normals"));
	config.generateTangents = toBool(properties("tangents"));
	config.trianglesOnly = toBool(properties("trianglesOnly"));
	config.passInvalidVectors = toBool(properties("passInvalidNormals"));
	const MeshImportResult result = meshImportFiles(inputFileName, config);
	meshImportNotifyUsedFiles(result);
	const uint32 partIndex = meshImportSelectIndex(result);
	const MeshImportPart &part = result.parts[partIndex];

	ModelDataFlags flags = ModelDataFlags::None;
	setFlags(flags, ModelDataFlags::Uvs2, !part.mesh->uvs().empty() || !part.mesh->uvs3().empty(), "uvs");
	setFlags(flags, ModelDataFlags::Normals, !part.mesh->normals().empty(), "normals");
	setFlags(flags, ModelDataFlags::Tangents, !part.mesh->tangents().empty(), "tangents");
	setFlags(flags, ModelDataFlags::Bones, !part.mesh->boneIndices().empty(), "bones");

	if (!part.mesh->uvs3().empty())
	{
		CAGE_ASSERT(part.mesh->uvs().empty());
		flags &= ~ModelDataFlags::Uvs2;
		flags |= ModelDataFlags::Uvs3;
	}
	if (any(flags & ModelDataFlags::Uvs2))
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using 2D uvs");
	if (any(flags & ModelDataFlags::Uvs3))
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using 3D uvs");

	if (none(flags & ModelDataFlags::Uvs2))
		part.mesh->uvs({});
	if (none(flags & ModelDataFlags::Uvs3))
		part.mesh->uvs3({});
	if (none(flags & ModelDataFlags::Normals))
		part.mesh->normals({});
	if (none(flags & ModelDataFlags::Tangents))
	{
		part.mesh->tangents({});
		part.mesh->bitangents({});
	}
	if (none(flags & ModelDataFlags::Bones))
	{
		part.mesh->boneIndices(PointerRange<Vec4i>());
		part.mesh->boneWeights(PointerRange<Vec4>());
	}

	ModelHeader dsm;
	detail::memset(&dsm, 0, sizeof(dsm));
	dsm.box = part.boundingBox;

	for (const auto &t : part.textures)
	{
		const String p = convertReferencePath(t.name);
		const uint32 n = HashString(p);
		switch (t.type)
		{
		case MeshImportTextureType::Albedo:
			dsm.textureNames[CAGE_SHADER_TEXTURE_ALBEDO] = n;
			break;
		case MeshImportTextureType::Special:
			dsm.textureNames[CAGE_SHADER_TEXTURE_SPECIAL] = n;
			break;
		case MeshImportTextureType::Normal:
			dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] = n;
			break;
		}
	}

	if (!part.shaderDepthName.empty())
		dsm.shaderDepthName = HashString(convertReferencePath(part.shaderDepthName));
	if (!part.shaderColorName.empty())
		dsm.shaderColorName = HashString(convertReferencePath(part.shaderColorName));

	dsm.renderFlags = part.renderFlags;
	dsm.renderLayer = part.renderLayer;
	dsm.skeletonBones = result.skeleton ? result.skeleton->bonesCount() : 0;
	dsm.materialSize = sizeof(MeshImportMaterial);

	const MeshImportMaterial &mat = part.material;
	printMaterial(dsm, mat);
	validateFlags(dsm, flags, part.renderFlags, mat);

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "bounding box: " + part.boundingBox);

	CAGE_LOG(SeverityEnum::Info, logComponentName, "serializing");
	AssetHeader h = initializeAssetHeader();
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;
	h.dependenciesCount += !!dsm.shaderDepthName;
	h.dependenciesCount += !!dsm.shaderColorName;

	cage::MemoryBuffer buffer;
	Serializer ser(buffer);
	ser << dsm;
	ser << mat;
	ser.write(part.mesh->exportBuffer());
	h.originalSize = buffer.size();
	Holder<PointerRange<char>> compressed = compress(buffer);
	h.compressedSize = compressed.size();

	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(bufferView(dsm.textureNames[i]));
	if (dsm.shaderDepthName)
		f->write(bufferView(dsm.shaderDepthName));
	if (dsm.shaderColorName)
		f->write(bufferView(dsm.shaderColorName));
	f->write(compressed);
	f->close();
}
