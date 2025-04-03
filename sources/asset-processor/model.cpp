#include <vector>

#include "processor.h"

#include <cage-core/flatSet.h>
#include <cage-core/hashString.h>
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-engine/shaderConventions.h>

MeshImportConfig meshImportConfig()
{
	MeshImportConfig config;
	config.rootPath = processor->inputDirectory;
	config.mergeParts = toBool(processor->property("bakeModel"));
	config.verticalFlipUv = true;
	config.verbose = true;
	return config;
}

namespace
{
	Mat3 axesMatrix(const String axes)
	{
		if (axes.empty() || axes == "+x+y+z")
			return Mat3();
		if (axes.length() != 6)
			CAGE_THROW_ERROR(Exception, "wrong axes definition: length (must be in format +x+y+z)");
		Mat3 result(0, 0, 0, 0, 0, 0, 0, 0, 0);
		int sign = 0;
		uint32 axesUsedCounts[3] = { 0, 0, 0 };
		for (uint32 i = 0; i < 6; i++)
		{
			char c = axes[i];
			if (i % 2 == 0)
			{ // signs
				if (c != '+' && c != '-')
					CAGE_THROW_ERROR(Exception, "wrong axes definition: signs (must be in format +x+y+z)");
				if (c == '+')
					sign = 1;
				else
					sign = -1;
			}
			else
			{ // axes
				uint32 out = i / 2;
				uint32 in = m;
				switch (c)
				{
					case 'x':
						axesUsedCounts[0]++;
						in = 0;
						break;
					case 'y':
						axesUsedCounts[1]++;
						in = 1;
						break;
					case 'z':
						axesUsedCounts[2]++;
						in = 2;
						break;
					default:
						CAGE_THROW_ERROR(Exception, "wrong axes definition: invalid axis (must be in format +x+y+z)");
				}
				result[in * 3 + out] = Real(sign);
			}
		}
		if (axesUsedCounts[0] != 1 || axesUsedCounts[1] != 1 || axesUsedCounts[2] != 1)
			CAGE_THROW_ERROR(Exception, "wrong axes definition: axes counts (must be in format +x+y+z)");
		return result;
	}

	void transformMesh(Mesh *msh, const Mat3 &axes, const Mat3 &axesScale)
	{
		std::vector<Vec3> p(msh->positions().begin(), msh->positions().end());
		std::vector<Vec3> n(msh->normals().begin(), msh->normals().end());
		for (Vec3 &i : p)
			i = axesScale * i;
		for (Vec3 &i : n)
			i = axes * i;
		msh->positions(p);
		msh->normals(n);
	}
}

Mat4 meshImportTransform(MeshImportResult &result)
{
	const Mat3 axes = axesMatrix(toLower(processor->property("axes")));
	const Mat3 axesScale = axes * toFloat(processor->property("scale"));
	if (axesScale == Mat3())
		return Mat4();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "using axes/scale conversion matrix: " + axesScale);
	for (auto &it : result.parts)
	{
		transformMesh(+it.mesh, axes, axesScale);
		it.boundingBox *= Mat4(axesScale);
	}
	return Mat4(axesScale);
}

void meshImportNotifyUsedFiles(const MeshImportResult &result)
{
	for (const String &p : result.paths)
		processor->writeLine(cage::String("use = ") + pathToRel(p, processor->inputDirectory));
}

uint32 meshImportSelectIndex(const MeshImportResult &result)
{
	if (result.parts.size() == 1 && processor->inputSpec.empty())
	{
		CAGE_LOG(SeverityEnum::Note, "assetProcessor", "using the first model, because it is the only model available");
		return 0;
	}

	if (isDigitsOnly(processor->inputSpec) && !processor->inputSpec.empty())
	{
		const uint32 n = toUint32(processor->inputSpec);
		if (n < result.parts.size())
		{
			CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "using model index " + n + ", because the input specifier is numeric");
			return n;
		}
		else
			CAGE_THROW_ERROR(Exception, "the input specifier is numeric, but the index is out of range");
	}

	FlatSet<uint32> candidates;
	for (uint32 modelIndex = 0; modelIndex < result.parts.size(); modelIndex++)
	{
		const String objName = result.parts[modelIndex].objectName;
		if (objName == processor->inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "considering model index " + modelIndex + ", because the model name is matching");
		}
		const String matName = result.parts[modelIndex].materialName;
		if (matName == processor->inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "considering model index " + modelIndex + ", because the material name matches");
		}
		const String comb = objName + "_" + matName;
		if (comb == processor->inputSpec)
		{
			candidates.insert(modelIndex);
			CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "considering model index " + modelIndex + ", because the combined name matches");
		}
	}

	switch (candidates.size())
	{
		case 0:
			CAGE_THROW_ERROR(Exception, "file does not contain requested model");
		case 1:
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "using model at index " + *candidates.begin());
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
		Bones = 1 << 1,
		Uvs2 = 1 << 2,
		Uvs3 = 1 << 3,
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
		bool requested = toBool(processor->property(name));
		if (requested && !available)
		{
			CAGE_LOG_THROW(cage::String("requested feature: ") + name);
			CAGE_THROW_ERROR(Exception, "requested feature not available");
		}
		if (requested)
		{
			flags |= idx;
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", cage::Stringizer() + "feature requested: " + name);
		}
	}

	void printMaterial(const ModelHeader &dsm, const MeshImportMaterial &mat)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "specialBase: " + mat.specialBase);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "specialMult: " + mat.specialMult);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "lighting: " + mat.lighting);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "animation: " + mat.animation);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "cutOut: " + any(dsm.renderFlags & MeshRenderFlags::CutOut));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "transparent: " + any(dsm.renderFlags & MeshRenderFlags::Transparent));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "fade: " + any(dsm.renderFlags & MeshRenderFlags::Fade));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "two sides: " + any(dsm.renderFlags & MeshRenderFlags::TwoSided));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "depth test: " + any(dsm.renderFlags & MeshRenderFlags::DepthTest));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "depth write: " + any(dsm.renderFlags & MeshRenderFlags::DepthWrite));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "shadow cast: " + any(dsm.renderFlags & MeshRenderFlags::ShadowCast));
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "lighting: " + any(dsm.renderFlags & MeshRenderFlags::Lighting));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i])
				CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void validateFlags(const ModelHeader &dsm, const ModelDataFlags flags, const MeshRenderFlags renderFlags, const MeshImportMaterial &mat)
	{
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && none(flags & (ModelDataFlags::Uvs2 | ModelDataFlags::Uvs3)))
			{
				if (dsm.shaderName)
					CAGE_LOG(SeverityEnum::Warning, "assetProcessor", "material has a texture but no uvs");
				else
					CAGE_THROW_ERROR(Exception, "material has a texture but no uvs");
			}
		}

		if (any(renderFlags & MeshRenderFlags::CutOut) + any(renderFlags & MeshRenderFlags::Transparent) + any(renderFlags & MeshRenderFlags::Fade) > 1)
			CAGE_THROW_ERROR(Exception, "material has multiple transparency flags (cutOut, transparent, fade)");

		if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && none(flags & ModelDataFlags::Normals))
			CAGE_THROW_ERROR(Exception, "model uses normal map texture but has no normals");
	}
}

void processModel()
{
	MeshImportConfig config = meshImportConfig();
	config.materialPathOverride = processor->property("material");
	config.materialNameAlternative = processor->inputSpec;
	config.generateNormals = toBool(processor->property("normals"));
	config.trianglesOnly = toBool(processor->property("trianglesOnly"));
	config.passInvalidVectors = toBool(processor->property("passInvalidNormals"));
	MeshImportResult result = meshImportFiles(processor->inputFileName, config);
	const Mat4 importTransform = meshImportTransform(result);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", "converting materials to cage format");
	meshImportConvertToCageFormats(result);
	meshImportNotifyUsedFiles(result);
	const uint32 partIndex = meshImportSelectIndex(result);
	const MeshImportPart &part = result.parts[partIndex];
	if (part.mesh->verticesCount() == 0)
		CAGE_THROW_ERROR(Exception, "the mesh is empty");

	ModelDataFlags flags = ModelDataFlags::None;
	setFlags(flags, ModelDataFlags::Uvs2, !part.mesh->uvs().empty() || !part.mesh->uvs3().empty(), "uvs");
	setFlags(flags, ModelDataFlags::Normals, !part.mesh->normals().empty(), "normals");
	setFlags(flags, ModelDataFlags::Bones, !part.mesh->boneIndices().empty(), "bones");

	if (!part.mesh->uvs3().empty())
	{
		CAGE_ASSERT(part.mesh->uvs().empty());
		flags &= ~ModelDataFlags::Uvs2;
		flags |= ModelDataFlags::Uvs3;
	}
	if (any(flags & ModelDataFlags::Uvs2))
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using 2D uvs");
	if (any(flags & ModelDataFlags::Uvs3))
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using 3D uvs");

	if (none(flags & ModelDataFlags::Uvs2))
		part.mesh->uvs({});
	if (none(flags & ModelDataFlags::Uvs3))
		part.mesh->uvs3({});
	if (none(flags & ModelDataFlags::Normals))
		part.mesh->normals({});
	if (none(flags & ModelDataFlags::Bones))
	{
		part.mesh->boneIndices(PointerRange<Vec4i>());
		part.mesh->boneWeights(PointerRange<Vec4>());
	}

	ModelHeader dsm;
	detail::memset(&dsm, 0, sizeof(dsm));
	dsm.importTransform = importTransform;
	dsm.box = part.boundingBox;

	for (const auto &t : part.textures)
	{
		const String p = processor->convertAssetPath(t.name);
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
			case MeshImportTextureType::Custom:
				dsm.textureNames[3] = n;
				break;
			default:
				break;
		}
	}

	if (!part.shaderName.empty())
		dsm.shaderName = HashString(processor->convertAssetPath(part.shaderName));

	dsm.renderFlags = part.renderFlags;
	dsm.renderLayer = part.renderLayer;
	dsm.skeletonBones = result.skeleton ? result.skeleton->bonesCount() : 0;
	dsm.materialSize = sizeof(MeshImportMaterial);

	const MeshImportMaterial &mat = part.material;
	printMaterial(dsm, mat);
	validateFlags(dsm, flags, part.renderFlags, mat);

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "bounding box: " + part.boundingBox);

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", "serializing");
	AssetHeader h = processor->initializeAssetHeader();
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;
	h.dependenciesCount += !!dsm.shaderName;

	cage::MemoryBuffer buffer;
	Serializer ser(buffer);
	ser << dsm;
	ser << mat;
	ser.write(part.mesh->exportBuffer());
	h.originalSize = buffer.size();
	Holder<PointerRange<char>> compressed = memoryCompress(buffer);
	h.compressedSize = compressed.size();

	Holder<File> f = writeFile(processor->outputFileName);
	f->write(bufferView(h));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(bufferView(dsm.textureNames[i]));
	if (dsm.shaderName)
		f->write(bufferView(dsm.shaderName));
	f->write(compressed);
	f->close();
}
