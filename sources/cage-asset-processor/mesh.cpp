#include "utility/assimp.h"
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/ini.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/pointer.h>
#include <cage-client/graphic/shaderConventions.h>
#include <cage-client/opengl.h>

namespace
{
	void setFlags(meshFlags &flags, meshFlags idx, bool available, const char *name)
	{
		bool requested = properties(name).toBool();
		if (requested && !available)
		{
			CAGE_LOG(severityEnum::Note, "exception", cage::string("requested feature: ") + name);
			CAGE_THROW_ERROR(exception, "requested feature not available");
		}
		if (requested)
		{
			flags |= idx;
			CAGE_LOG(severityEnum::Info, logComponentName, cage::string() + "feature requested: " + name);
		}
	}

	void loadTextureExternal(meshHeaderStruct &dsm, iniClass *ini, const string &type, uint32 usage)
	{
		CAGE_ASSERT_RUNTIME(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, type);
		string n = ini->getString("textures", type);
		if (n.empty())
			return;
		n = pathJoin(pathExtractPath(inputName), n);
		dsm.textureNames[usage] = hashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + type + ", usage " + usage);
	}

	const bool loadTextureAssimp(aiMaterial *m, meshHeaderStruct &dsm, aiTextureType tt, uint32 usage)
	{
		CAGE_ASSERT_RUNTIME(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, tt);
		uint32 texCount = m->GetTextureCount(tt);
		if (texCount == 0)
			return false;
		if (texCount > 1)
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "material has multiple (" + texCount + ") textures of type " + (uint32)tt + ", usage " + usage);
		aiString texAsName;
		m->GetTexture(tt, 0, &texAsName, nullptr, nullptr, nullptr, nullptr, nullptr);
		cage::string n = pathJoin(pathExtractPath(inputName), cage::string(texAsName.C_Str()));
		dsm.textureNames[usage] = hashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + (uint32)tt + ", usage " + usage);
		return true;
	}

	void printMaterial(const meshHeaderStruct &dsm, const meshHeaderStruct::materialDataStruct &mat)
	{
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "specialBase: " + mat.specialBase);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "specialMult: " + mat.specialMult);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "opacity texture: " + ((dsm.flags & meshFlags::OpacityTexture) == meshFlags::OpacityTexture));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "transparency: " + ((dsm.flags & meshFlags::Transparency) == meshFlags::Transparency));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "translucency: " + ((dsm.flags & meshFlags::Translucency) == meshFlags::Translucency));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "two sides: " + ((dsm.flags & meshFlags::TwoSided) == meshFlags::TwoSided));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "depth test: " + ((dsm.flags & meshFlags::DepthTest) == meshFlags::DepthTest));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "lighting: " + ((dsm.flags & meshFlags::Lighting) == meshFlags::Lighting));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "shadow cast: " + ((dsm.flags & meshFlags::ShadowCast) == meshFlags::ShadowCast));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i] == 0)
				continue;
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void loadMaterialExternal(meshHeaderStruct &dsm, meshHeaderStruct::materialDataStruct &mat, string path)
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "using external (.cpm) material");

		writeLine(string("use = ") + path);
		path = pathJoin(inputDirectory, path);

		holder<iniClass> ini = newIni();
		ini->load(path);

		mat.albedoBase = vec4(
			toVec3(ini->getString("base", "albedo", "0, 0, 0")),
			ini->getString("base", "opacity", "1").toFloat()
		);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.flags |= meshFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.flags |= meshFlags::Translucency;

		mat.specialBase = vec4(
			ini->getString("base", "roughness", "0").toFloat(),
			ini->getString("base", "metalness", "0").toFloat(),
			ini->getString("base", "emissive", "0").toFloat(),
			ini->getString("base", "mask", "0").toFloat()
		);

		mat.albedoMult = vec4(toVec3(ini->getString("mult", "albedo", "1, 1, 1")), 1);

		mat.specialMult = vec4(
			ini->getString("mult", "roughness", "1").toFloat(),
			ini->getString("mult", "metalness", "1").toFloat(),
			ini->getString("mult", "emissive", "1").toFloat(),
			ini->getString("mult", "mask", "1").toFloat()
		);

		loadTextureExternal(dsm, ini.get(), "albedo", CAGE_SHADER_TEXTURE_ALBEDO);
		loadTextureExternal(dsm, ini.get(), "special", CAGE_SHADER_TEXTURE_SPECIAL);
		loadTextureExternal(dsm, ini.get(), "normal", CAGE_SHADER_TEXTURE_NORMAL);

		for (uint32 i = 0, e = ini->itemCount("flags"); i != e; i++)
		{
			string n = ini->item("flags", i);
			string v = ini->get("flags", n);
			if (v == "opacity-texture")
			{
				dsm.flags |= meshFlags::OpacityTexture;
				continue;
			}
			if (v == "two-sided")
			{
				dsm.flags |= meshFlags::TwoSided;
				continue;
			}
			if (v == "transparency")
			{
				dsm.flags |= meshFlags::Transparency;
				continue;
			}
			if (v == "translucency")
			{
				dsm.flags |= meshFlags::Translucency;
				continue;
			}
			if (v == "no-depth-test")
			{
				dsm.flags &= ~meshFlags::DepthTest;
				continue;
			}
			if (v == "no-depth-write")
			{
				dsm.flags &= ~meshFlags::DepthWrite;
				continue;
			}
			if (v == "no-lighting")
			{
				dsm.flags &= ~meshFlags::Lighting;
				continue;
			}
			if (v == "no-shadow-cast")
			{
				dsm.flags &= ~meshFlags::ShadowCast;
				continue;
			}
			CAGE_LOG(severityEnum::Note, "exception", string() + "specified flag: '" + v + "'");
			CAGE_THROW_ERROR(exception, "unknown material flag");
		}
	}

	void loadMaterialAssimp(const aiScene *scene, const aiMesh *am, meshHeaderStruct &dsm, meshHeaderStruct::materialDataStruct &mat)
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "converting assimp material");

		CAGE_ASSERT_RUNTIME(am->mMaterialIndex < scene->mNumMaterials, "material index out of range", am->mMaterialIndex, scene->mNumMaterials);
		aiMaterial *m = scene->mMaterials[am->mMaterialIndex];
		if (!m)
			CAGE_THROW_ERROR(exception, "material is null");

		aiString matName;
		m->Get(AI_MATKEY_NAME, matName);
		if (matName.length > 0)
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "material name: '" + matName.C_Str() + "'");

		m->Get(AI_MATKEY_OPACITY, mat.albedoBase[3].value);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.flags |= meshFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.flags |= meshFlags::Translucency;

		if (loadTextureAssimp(m, dsm, aiTextureType_DIFFUSE, CAGE_SHADER_TEXTURE_ALBEDO))
		{
			int flg = 0;
			m->Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), flg);
			if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha || mat.albedoBase[3] <= 1e-7)
			{
				dsm.flags |= meshFlags::OpacityTexture;
				if ((dsm.flags & meshFlags::Transparency) == meshFlags::None)
					dsm.flags |= meshFlags::Translucency;
			}
		}
		else
		{
			aiColor3D color = aiColor3D(1, 1, 1);
			m->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mat.albedoBase = vec4(vec3(color.r, color.g, color.b), mat.albedoBase[3]);

			if (mat.albedoBase[3] < 1e-7)
				dsm.flags |= meshFlags::Transparency;
			else if (mat.albedoBase[3] < 1 - 1e-7)
				dsm.flags |= meshFlags::Translucency;
		}

		if (!loadTextureAssimp(m, dsm, aiTextureType_SHININESS, CAGE_SHADER_TEXTURE_SPECIAL))
		{
			float shininess = -1;
			m->Get(AI_MATKEY_SHININESS, shininess);
			if (shininess >= 0)
				mat.specialBase[0] = sqrt(2 / (2 + shininess)); // convert shininess to roughness
		}

		loadTextureAssimp(m, dsm, aiTextureType_HEIGHT, CAGE_SHADER_TEXTURE_NORMAL);
		loadTextureAssimp(m, dsm, aiTextureType_NORMALS, CAGE_SHADER_TEXTURE_NORMAL);

		{
			int twosided = false;
			m->Get(AI_MATKEY_TWOSIDED, twosided);
			if (twosided)
				dsm.flags |= meshFlags::TwoSided;
		}
	}

	void loadMaterial(const aiScene *scene, const aiMesh *am, meshHeaderStruct &dsm, meshHeaderStruct::materialDataStruct &mat)
	{
		string path = properties("override_material");
		if (!path.empty())
		{
			path = pathJoin(pathExtractPath(inputFile), path);
			if (!pathExists(pathJoin(inputDirectory, path)))
				CAGE_THROW_ERROR(exception, "overriden material path does not exist");
			loadMaterialExternal(dsm, mat, path);
			return;
		}

		path = inputFile;
		aiMaterial *m = scene->mMaterials[am->mMaterialIndex];
		if (m)
		{
			aiString matName;
			m->Get(AI_MATKEY_NAME, matName);
			if (matName.length)
				path += string() + "_" + matName.C_Str();
			else if (!inputSpec.empty())
				path += string() + "_" + inputSpec;
		}
		else
		{
			if (!inputSpec.empty())
				path += string() + "_" + inputSpec;
		}
		path += ".cpm";

		CAGE_LOG(severityEnum::Info, logComponentName, string() + "implicitly looking for external material at '" + path + "'");

		if (pathExists(pathJoin(inputDirectory, path)))
		{
			loadMaterialExternal(dsm, mat, path);
			return;
		}

		loadMaterialAssimp(scene, am, dsm, mat);
	}

	void validateFlags(const meshHeaderStruct &dsm, const meshHeaderStruct::materialDataStruct &mat)
	{
		if ((dsm.flags & meshFlags::OpacityTexture) == meshFlags::OpacityTexture && (dsm.flags & (meshFlags::Translucency | meshFlags::Transparency)) == meshFlags::None)
			CAGE_THROW_ERROR(exception, "material flags contains opacity texture, but neither translucency nor transparency is set");
		if ((dsm.flags & meshFlags::Translucency) == meshFlags::Translucency && (dsm.flags & meshFlags::Transparency) == meshFlags::Transparency)
			CAGE_THROW_ERROR(exception, "material flags transparency and translucency are mutually exclusive");
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && (dsm.flags & meshFlags::Uvs) == meshFlags::None)
				CAGE_THROW_ERROR(exception, "material has textures, but uvs are missing");
		}
		if ((dsm.flags & meshFlags::Tangents) == meshFlags::Tangents && (dsm.flags & meshFlags::Uvs) == meshFlags::None)
			CAGE_THROW_ERROR(exception, "tangents are exported, but uvs are missing");
		if ((dsm.flags & meshFlags::Tangents) == meshFlags::Tangents && (dsm.flags & meshFlags::Normals) == meshFlags::None)
			CAGE_THROW_ERROR(exception, "tangents are exported, but normals are missing");
		if ((dsm.flags & meshFlags::ShadowCast) == meshFlags::ShadowCast && ((dsm.flags & meshFlags::DepthWrite) == meshFlags::None))
			CAGE_THROW_ERROR(exception, "shadow cast is enabled, but depth write is disabled");
	}
}

extern const uint32 assimpDefaultLoadFlags;

void processMesh()
{
	holder<assimpContextClass> context;
	const aiScene *scene = nullptr;

	{
		uint32 flags = assimpDefaultLoadFlags;
		if (properties("export_normal").toBool())
			flags |= aiProcess_GenSmoothNormals;
		if (properties("export_tangent").toBool())
			flags |= aiProcess_CalcTangentSpace;
		context = newAssimpContext(flags);
		scene = context->getScene();
	}

	const aiMesh *am = scene->mMeshes[context->selectMesh()];

	if (am->GetNumUVChannels() > 1)
		CAGE_LOG(severityEnum::Warning, logComponentName, "multiple uv channels are not supported - using only the first");

	meshHeaderStruct dsm;
	memset(&dsm, 0, sizeof(dsm));
	dsm.materialSize = sizeof(meshHeaderStruct::materialDataStruct);
	dsm.flags = meshFlags::DepthTest | meshFlags::DepthWrite | meshFlags::Lighting | meshFlags::ShadowCast;

	uint32 indicesPerPrimitive = 0;
	switch (am->mPrimitiveTypes)
	{
	case aiPrimitiveType_POINT: dsm.primitiveType = GL_POINTS; indicesPerPrimitive = 1; break;
	case aiPrimitiveType_LINE: dsm.primitiveType = GL_LINES; indicesPerPrimitive = 2; break;
	case aiPrimitiveType_TRIANGLE: dsm.primitiveType = GL_TRIANGLES; indicesPerPrimitive = 3; break;
	default: CAGE_THROW_ERROR(exception, "mesh has invalid primitive type");
	}
	dsm.verticesCount = am->mNumVertices;
	dsm.indicesCount = am->mNumFaces * indicesPerPrimitive;

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "primitive type: " + dsm.primitiveType + ", indices per primitive: " + indicesPerPrimitive);
	CAGE_LOG(severityEnum::Info, logComponentName, cage::string() + "vertices count: " + dsm.verticesCount);
	CAGE_LOG(severityEnum::Info, logComponentName, cage::string() + "indices count: " + dsm.indicesCount);

	if (properties("export_bones").toBool()) // todo
		CAGE_THROW_CRITICAL(notImplementedException, "exporting bones is not yet implemented");
	// note: when exporting with bones, the bounding box must encapsulate the mesh in all possible poses

	setFlags(dsm.flags, meshFlags::Uvs, am->GetNumUVChannels() > 0, "export_uv");
	setFlags(dsm.flags, meshFlags::Normals, am->HasNormals(), "export_normal");
	setFlags(dsm.flags, meshFlags::Tangents, am->HasTangentsAndBitangents(), "export_tangent");
	setFlags(dsm.flags, meshFlags::Bones, am->HasBones(), "export_bones");

	meshHeaderStruct::materialDataStruct mat;
	memset(&mat, 0, sizeof(mat));
	mat.albedoBase = vec4(0, 0, 0, 1);
	mat.specialBase = vec4(0, 0, 0, 0);
	mat.albedoMult = vec4(1, 1, 1, 1);
	mat.specialMult = vec4(1, 1, 1, 1);

	loadMaterial(scene, am, dsm, mat);
	printMaterial(dsm, mat);
	validateFlags(dsm, mat);

	float scale = properties("scale").toFloat();
	const cage::uintPtr dataSize = dsm.vertexSize() * dsm.verticesCount + sizeof(uint32) * dsm.indicesCount;
	cage::memoryBuffer dataBuffer(dataSize);
	pointer ptr = dataBuffer.data();

	dsm.box = aabb();
	for (uint32 i = 0; i < dsm.verticesCount; i++)
	{
		vec3 p = (*(cage::vec3*)&(am->mVertices[i])) * scale;
		*(cage::vec3*)ptr.asVoid = p;
		ptr += sizeof(cage::vec3);
		dsm.box += aabb(p);
	}
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "bounding box: " + dsm.box);

	if (dsm.uvs())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			*(cage::vec2*)ptr.asVoid = cage::vec2(*(cage::vec3*)&(am->mTextureCoords[0][i]));
			ptr += sizeof(cage::vec2);
		}
	}

	if (dsm.normals())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			*(cage::vec3*)ptr.asVoid = *(cage::vec3*)&(am->mNormals[i]);
			ptr += sizeof(cage::vec3);
		}
	}

	if (dsm.tangents())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			*(cage::vec3*)ptr.asVoid = *(cage::vec3*)&(am->mTangents[i]);
			ptr += sizeof(cage::vec3);
		}
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			*(cage::vec3*)ptr.asVoid = *(cage::vec3*)&(am->mBitangents[i]);
			ptr += sizeof(cage::vec3);
		}
	}

	for (uint32 i = 0; i < am->mNumFaces; i++)
	{
		for (uint32 j = 0; j < indicesPerPrimitive; j++)
		{
			*(cage::uint32*)ptr.asVoid = numeric_cast<uint32>(am->mFaces[i].mIndices[j]);
			ptr += sizeof(cage::uint32);
		}
	}

	CAGE_ASSERT_RUNTIME(ptr == pointer(dataBuffer.data()) + dataSize, ptr, dataBuffer.data(), dataSize);

	assetHeaderStruct h = initializeAssetHeaderStruct();
	h.originalSize = sizeof(dsm) + sizeof(mat) + numeric_cast<uint32>(dataSize);
	if (dsm.skeletonName)
		h.dependenciesCount++;
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	if (dsm.skeletonName)
		f->write(&dsm.skeletonName, sizeof(uint32));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(&dsm.textureNames[i], sizeof(uint32));
	f->write(&dsm, sizeof(dsm));
	f->write(&mat, sizeof(mat));
	f->write(dataBuffer.data(), dataSize);
	f->close();
}