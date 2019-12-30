#include "utility/assimp.h"
#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-engine/graphics/shaderConventions.h>
#include <cage-engine/opengl.h>

vec2 convertSpecularToSpecial(const vec3 &spec);

namespace
{
	void setFlags(MeshDataFlags &flags, MeshDataFlags idx, bool available, const char *name)
	{
		bool requested = properties(name).toBool();
		if (requested && !available)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", cage::string("requested feature: ") + name);
			CAGE_THROW_ERROR(Exception, "requested feature not available");
		}
		if (requested)
		{
			flags |= idx;
			CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "feature requested: " + name);
		}
	}

	void loadTextureCage(const string &pathBase, MeshHeader &dsm, Ini *ini, const string &type, uint32 usage)
	{
		CAGE_ASSERT(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, type);
		string n = ini->getString("textures", type);
		if (n.empty())
			return;
		n = pathJoin(pathBase, n);
		dsm.textureNames[usage] = HashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + type + ", usage " + usage);
	}

	bool loadTextureAssimp(aiMaterial *m, MeshHeader &dsm, aiTextureType tt, uint32 usage)
	{
		CAGE_ASSERT(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, tt);
		uint32 texCount = m->GetTextureCount(tt);
		if (texCount == 0)
			return false;
		if (texCount > 1)
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "material has multiple (" + texCount + ") textures of type " + (uint32)tt + ", usage " + usage);
		aiString texAsName;
		m->GetTexture(tt, 0, &texAsName, nullptr, nullptr, nullptr, nullptr, nullptr);
		cage::string tn = texAsName.C_Str();
		if (tn.isPattern("//", "", ""))
			tn = string() + "./" + tn.subString(2, cage::m);
		cage::string n = pathJoin(pathExtractPath(inputName), tn);
		dsm.textureNames[usage] = HashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + (uint32)tt + ", usage " + usage);
		return true;
	}

	void printMaterial(const MeshHeader &dsm, const MeshHeader::materialData &mat)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "specialBase: " + mat.specialBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "specialMult: " + mat.specialMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "opacity texture: " + ((dsm.renderFlags & MeshRenderFlags::OpacityTexture) == MeshRenderFlags::OpacityTexture));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "transparency: " + ((dsm.renderFlags & MeshRenderFlags::Transparency) == MeshRenderFlags::Transparency));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "translucency: " + ((dsm.renderFlags & MeshRenderFlags::Translucency) == MeshRenderFlags::Translucency));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "lighting: " + ((dsm.renderFlags & MeshRenderFlags::Lighting) == MeshRenderFlags::Lighting));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "two sides: " + ((dsm.renderFlags & MeshRenderFlags::TwoSided) == MeshRenderFlags::TwoSided));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "depth test: " + ((dsm.renderFlags & MeshRenderFlags::DepthTest) == MeshRenderFlags::DepthTest));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "depth write: " + ((dsm.renderFlags & MeshRenderFlags::DepthWrite) == MeshRenderFlags::DepthWrite));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "velocity write: " + ((dsm.renderFlags & MeshRenderFlags::VelocityWrite) == MeshRenderFlags::VelocityWrite));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "shadow cast: " + ((dsm.renderFlags & MeshRenderFlags::ShadowCast) == MeshRenderFlags::ShadowCast));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i] == 0)
				continue;
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void loadMaterialCage(MeshHeader &dsm, MeshHeader::materialData &mat, string path)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using cage (.cpm) material");

		writeLine(string("use = ") + path);

		Holder<Ini> ini = newIni();
		ini->load(pathJoin(inputDirectory, path));

		mat.albedoBase = vec4(
			vec3::parse(ini->getString("base", "albedo", "0, 0, 0")),
			ini->getFloat("base", "opacity", 1)
		);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.renderFlags |= MeshRenderFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.renderFlags |= MeshRenderFlags::Translucency;

		mat.specialBase = vec4(
			ini->getFloat("base", "roughness", 0),
			ini->getFloat("base", "metallic", 0),
			ini->getFloat("base", "emission", 0),
			ini->getFloat("base", "mask", 0)
		);

		mat.albedoMult = vec4(vec3::parse(ini->getString("mult", "albedo", "1, 1, 1")), 1);

		mat.specialMult = vec4(
			ini->getFloat("mult", "roughness", 1),
			ini->getFloat("mult", "metallic", 1),
			ini->getFloat("mult", "emission", 1),
			ini->getFloat("mult", "mask", 1)
		);

		string pathBase = pathExtractPath(path);
		loadTextureCage(pathBase, dsm, ini.get(), "albedo", CAGE_SHADER_TEXTURE_ALBEDO);
		loadTextureCage(pathBase, dsm, ini.get(), "special", CAGE_SHADER_TEXTURE_SPECIAL);
		loadTextureCage(pathBase, dsm, ini.get(), "normal", CAGE_SHADER_TEXTURE_NORMAL);

		for (const string &n : ini->items("flags"))
		{
			string v = ini->getString("flags", n);
			if (v == "opacityTexture")
			{
				dsm.renderFlags |= MeshRenderFlags::OpacityTexture;
				continue;
			}
			if (v == "twoSided")
			{
				dsm.renderFlags |= MeshRenderFlags::TwoSided;
				continue;
			}
			if (v == "transparency")
			{
				dsm.renderFlags |= MeshRenderFlags::Transparency;
				continue;
			}
			if (v == "translucency")
			{
				dsm.renderFlags |= MeshRenderFlags::Translucency;
				continue;
			}
			if (v == "noDepthTest")
			{
				dsm.renderFlags &= ~MeshRenderFlags::DepthTest;
				continue;
			}
			if (v == "noDepthWrite")
			{
				dsm.renderFlags &= ~MeshRenderFlags::DepthWrite;
				continue;
			}
			if (v == "noVelocityWrite")
			{
				dsm.renderFlags &= ~MeshRenderFlags::VelocityWrite;
				continue;
			}
			if (v == "noLighting")
			{
				dsm.renderFlags &= ~MeshRenderFlags::Lighting;
				continue;
			}
			if (v == "noShadowCast")
			{
				dsm.renderFlags &= ~MeshRenderFlags::ShadowCast;
				continue;
			}
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "specified flag: '" + v + "'");
			CAGE_THROW_ERROR(Exception, "unknown material flag");
		}

		{
			string s, t, v;
			if (ini->anyUnused(s, t, v))
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "section: " + s);
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "item: " + t);
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "value: " + v);
				CAGE_THROW_ERROR(Exception, "unused material property");
			}
		}
	}

	void loadMaterialAssimp(const aiScene *scene, const aiMesh *am, MeshHeader &dsm, MeshHeader::materialData &mat)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "converting assimp material");

		CAGE_ASSERT(am->mMaterialIndex < scene->mNumMaterials, "material index out of range", am->mMaterialIndex, scene->mNumMaterials);
		aiMaterial *m = scene->mMaterials[am->mMaterialIndex];
		if (!m)
			CAGE_THROW_ERROR(Exception, "material is null");

		aiString matName;
		m->Get(AI_MATKEY_NAME, matName);
		if (matName.length > 0)
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "material name: '" + matName.C_Str() + "'");

		// opacity
		m->Get(AI_MATKEY_OPACITY, mat.albedoBase[3].value);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.renderFlags |= MeshRenderFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.renderFlags |= MeshRenderFlags::Translucency;

		// albedo
		if (loadTextureAssimp(m, dsm, aiTextureType_DIFFUSE, CAGE_SHADER_TEXTURE_ALBEDO))
		{
			int flg = 0;
			m->Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), flg);
			if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha || mat.albedoBase[3] <= 1e-7)
			{
				dsm.renderFlags |= MeshRenderFlags::OpacityTexture;
				if ((dsm.renderFlags & MeshRenderFlags::Transparency) == MeshRenderFlags::None)
					dsm.renderFlags |= MeshRenderFlags::Translucency;
			}
		}
		else
		{
			aiColor3D color = aiColor3D(1);
			m->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mat.albedoBase = vec4(conv(color), mat.albedoBase[3]);

			if (mat.albedoBase[3] < 1e-7)
				dsm.renderFlags |= MeshRenderFlags::Transparency;
			else if (mat.albedoBase[3] < 1 - 1e-7)
				dsm.renderFlags |= MeshRenderFlags::Translucency;
		}

		// special
		if (!loadTextureAssimp(m, dsm, aiTextureType_SPECULAR, CAGE_SHADER_TEXTURE_SPECIAL))
		{
			aiColor3D spec = aiColor3D(0);
			m->Get(AI_MATKEY_COLOR_SPECULAR, spec);
			if (conv(spec) != vec3(0))
			{ // convert specular color to special material
				vec2 s = convertSpecularToSpecial(conv(spec));
				mat.specialBase[0] = s[0];
				mat.specialBase[1] = s[1];
			}
			else
			{ // convert shininess to roughness
				float shininess = -1;
				m->Get(AI_MATKEY_SHININESS, shininess);
				if (shininess >= 0)
					mat.specialBase[0] = sqrt(2 / (2 + shininess));
			}
		}

		// normal map
		loadTextureAssimp(m, dsm, aiTextureType_HEIGHT, CAGE_SHADER_TEXTURE_NORMAL);
		loadTextureAssimp(m, dsm, aiTextureType_NORMALS, CAGE_SHADER_TEXTURE_NORMAL);

		// two sided
		{
			int twosided = false;
			m->Get(AI_MATKEY_TWOSIDED, twosided);
			if (twosided)
				dsm.renderFlags |= MeshRenderFlags::TwoSided;
		}
	}

	void loadMaterial(const aiScene *scene, const aiMesh *am, MeshHeader &dsm, MeshHeader::materialData &mat)
	{
		string path = properties("material");
		if (!path.empty())
		{
			path = pathJoin(pathExtractPath(inputFile), path);
			if (!pathIsFile(pathJoin(inputDirectory, path)))
				CAGE_THROW_ERROR(Exception, "explicitly given material path does not exist");
			loadMaterialCage(dsm, mat, path);
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

		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "looking for implicit material at '" + path + "'");

		if (pathIsFile(pathJoin(inputDirectory, path)))
		{
			loadMaterialCage(dsm, mat, path);
			return;
		}

		loadMaterialAssimp(scene, am, dsm, mat);
	}

	void validateFlags(const MeshHeader &dsm, const MeshHeader::materialData &mat)
	{
		if ((dsm.renderFlags & MeshRenderFlags::OpacityTexture) == MeshRenderFlags::OpacityTexture && (dsm.renderFlags & (MeshRenderFlags::Translucency | MeshRenderFlags::Transparency)) == MeshRenderFlags::None)
			CAGE_THROW_ERROR(Exception, "material flags contains opacity texture, but neither translucency nor transparency is set");
		if ((dsm.renderFlags & MeshRenderFlags::Translucency) == MeshRenderFlags::Translucency && (dsm.renderFlags & MeshRenderFlags::Transparency) == MeshRenderFlags::Transparency)
			CAGE_THROW_ERROR(Exception, "material flags transparency and translucency are mutually exclusive");
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && (dsm.flags & MeshDataFlags::Uvs) == MeshDataFlags::None)
				CAGE_THROW_ERROR(Exception, "material has textures, but uvs are missing");
		}
		if ((dsm.flags & MeshDataFlags::Tangents) == MeshDataFlags::Tangents && (dsm.flags & MeshDataFlags::Uvs) == MeshDataFlags::None)
			CAGE_THROW_ERROR(Exception, "tangents are exported, but uvs are missing");
		if ((dsm.flags & MeshDataFlags::Tangents) == MeshDataFlags::Tangents && (dsm.flags & MeshDataFlags::Normals) == MeshDataFlags::None)
			CAGE_THROW_ERROR(Exception, "tangents are exported, but normals are missing");
	}

	void loadSkeletonName(MeshHeader &dsm)
	{
		string n = properties("skeleton");
		if (!dsm.bones())
		{
			if (!n.empty())
				CAGE_THROW_ERROR(Exception, "cannot override skeleton for a mesh that has no bones");
			return;
		}
		if (n.empty())
			n = inputFile + ";skeleton";
		else
			n = pathJoin(pathExtractPath(inputName), n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "using skeleton name: '" + n + "'");
		dsm.skeletonName = HashString(n.c_str());
		writeLine(string("ref = ") + n);
	}

	vec3 fixUnitVector(vec3 n, const char *name)
	{
		if (abs(lengthSquared(n) - 1) > 1e-3)
		{
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "fixing denormalized " + name + ": " + n);
			n = normalize(n);
		}
		if (!n.valid())
		{
			static bool passInvalid = properties("passInvalidNormals").toBool();
			if (passInvalid)
			{
				CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "pass invalid " + name + ": " + n);
				n = vec3();
			}
			else
				CAGE_THROW_ERROR(Exception, name);
		}
		return n;
	}
}

void processMesh()
{
	Holder<assimpContextClass> context;
	{
		uint32 addFlags = 0;
		if (properties("normals").toBool())
			addFlags |= aiProcess_GenSmoothNormals;
		if (properties("tangents").toBool())
			addFlags |= aiProcess_CalcTangentSpace;
		context = newAssimpContext(addFlags, 0);
	}
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectMesh()];

	if (am->GetNumUVChannels() > 1)
		CAGE_LOG(SeverityEnum::Warning, logComponentName, "multiple uv channels are not supported - using only the first");

	MeshHeader dsm;
	memset(&dsm, 0, sizeof(dsm));
	dsm.materialSize = sizeof(MeshHeader::materialData);
	dsm.renderFlags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::VelocityWrite | MeshRenderFlags::Lighting | MeshRenderFlags::ShadowCast;

	uint32 indicesPerPrimitive = 0;
	switch (am->mPrimitiveTypes)
	{
	case aiPrimitiveType_POINT: dsm.primitiveType = GL_POINTS; indicesPerPrimitive = 1; break;
	case aiPrimitiveType_LINE: dsm.primitiveType = GL_LINES; indicesPerPrimitive = 2; break;
	case aiPrimitiveType_TRIANGLE: dsm.primitiveType = GL_TRIANGLES; indicesPerPrimitive = 3; break;
	default: CAGE_THROW_ERROR(Exception, "mesh has invalid primitive type");
	}
	dsm.verticesCount = am->mNumVertices;
	dsm.indicesCount = am->mNumFaces * indicesPerPrimitive;

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "primitive type: " + dsm.primitiveType + ", indices per primitive: " + indicesPerPrimitive);
	CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "vertices count: " + dsm.verticesCount);
	CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "indices count: " + dsm.indicesCount);

	setFlags(dsm.flags, MeshDataFlags::Uvs, am->GetNumUVChannels() > 0, "uvs");
	setFlags(dsm.flags, MeshDataFlags::Normals, am->HasNormals(), "normals");
	setFlags(dsm.flags, MeshDataFlags::Tangents, am->HasTangentsAndBitangents(), "tangents");
	setFlags(dsm.flags, MeshDataFlags::Bones, am->HasBones(), "bones");

	loadSkeletonName(dsm);

	dsm.instancesLimitHint = properties("instancesLimit").toUint32();

	MeshHeader::materialData mat;
	memset(&mat, 0, sizeof(mat));
	mat.albedoBase = vec4(0, 0, 0, 1);
	mat.specialBase = vec4(0, 0, 0, 0);
	mat.albedoMult = vec4(1, 1, 1, 1);
	mat.specialMult = vec4(1, 1, 1, 1);

	loadMaterial(scene, am, dsm, mat);
	printMaterial(dsm, mat);
	validateFlags(dsm, mat);

	if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && !dsm.normals())
		CAGE_THROW_ERROR(Exception, "mesh uses normal map texture but has no normals");
	if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && !dsm.tangents())
		CAGE_THROW_ERROR(Exception, "mesh uses normal map texture but has no tangents");

	cage::MemoryBuffer dataBuffer;
	dataBuffer.reserve(dsm.vertexSize() * dsm.verticesCount);
	cage::Serializer ser(dataBuffer);
	cage::Serializer dsmPlaceholder = ser.placeholder(sizeof(dsm));

	dsm.box = aabb();
	mat3 axes = axesMatrix();
	mat3 axesScale = axesScaleMatrix();
	for (uint32 i = 0; i < dsm.verticesCount; i++)
	{
		vec3 p = axesScale * conv(am->mVertices[i]);
		ser << p;
		dsm.box += aabb(p);
	}
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "bounding box: " + dsm.box);

	if (dsm.normals())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			vec3 n = axes * conv(am->mNormals[i]);
			ser << fixUnitVector(n, "normal");
		}
	}

	if (dsm.tangents())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			vec3 n = axes * conv(am->mTangents[i]);
			ser << fixUnitVector(n, "tangent");
		}
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			vec3 n = axes * conv(am->mBitangents[i]);
			ser << fixUnitVector(n, "bitangent");
		}
	}

	if (dsm.bones())
	{
		// enlarge bounding box
		{
			vec3 c = dsm.box.center();
			vec3 a = dsm.box.a - c;
			vec3 b = dsm.box.b - c;
			real s = 3;
			dsm.box = aabb(a * s + c, b * s + c);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "enlarged bounding box: " + dsm.box);
		}
		CAGE_ASSERT(am->mNumBones > 0);
		Holder<assimpSkeletonClass> skeleton = context->skeleton();
		dsm.skeletonBones = skeleton->bonesCount();
		Serializer ser2 = ser.placeholder((sizeof(uint16) + sizeof(float)) * 4 * dsm.verticesCount);
		uint16 *boneIndices = (uint16*)ser2.advance(sizeof(uint16) * 4 * dsm.verticesCount);
		float *boneWeights = (float*)ser2.advance(sizeof(float) * 4 * dsm.verticesCount);
		// initialize with empty values
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			for (uint32 j = 0; j < 4; j++)
			{
				boneIndices[i * 4 + j] = m;
				boneWeights[i * 4 + j] = 0;
			}
		}
		// copy the values from assimp
		for (uint32 boneIndex = 0; boneIndex < am->mNumBones; boneIndex++)
		{
			aiBone *bone = am->mBones[boneIndex];
			CAGE_ASSERT(bone);
			uint16 boneId = skeleton->index(bone);
			CAGE_ASSERT(boneId != m);
			for (uint32 weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++)
			{
				aiVertexWeight *w = bone->mWeights + weightIndex;
				bool ok = false;
				for (uint32 i = 0; i < 4; i++)
				{
					if (boneIndices[w->mVertexId * 4 + i] == m)
					{
						boneIndices[w->mVertexId * 4 + i] = boneId;
						boneWeights[w->mVertexId * 4 + i] = w->mWeight;
						ok = true;
						break;
					}
				}
				CAGE_ASSERT(ok, "single vertex may not be affected by more than four bones");
			}
		}
		// validate
		uint32 maxBoneId = 0;
		for (uint32 i = 0; i < dsm.verticesCount; i++)
		{
			float sum = 0;
			for (uint32 j = 0; j < 4; j++)
			{
				if (boneIndices[i * 4 + j] == m)
				{
					CAGE_ASSERT(boneWeights[i * 4 + j] == 0, i, j);
					boneIndices[i * 4 + j] = 0; // prevent shader from accessing invalid memory
				}
				sum += boneWeights[i * 4 + j];
				maxBoneId = max(maxBoneId, boneIndices[i * 4 + j] + 1u);
			}
			// renormalize weights
			if (cage::abs(sum - 1) > 1e-3 && sum > 1e-3)
			{
				float f = 1 / sum;
				CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "renormalizing bone weights for " + i + "th vertex by factor " + f);
				for (uint32 j = 0; j < 4; j++)
					boneWeights[i * 4 + j] *= f;
			}
		}
		CAGE_ASSERT(maxBoneId <= dsm.skeletonBones, maxBoneId, dsm.skeletonBones);
	}

	if (dsm.uvs())
	{
		if (am->mNumUVComponents[0] == 3)
		{
			dsm.uvDimension = 3;
			for (uint32 i = 0; i < dsm.verticesCount; i++)
				ser << conv(am->mTextureCoords[0][i]);
		}
		else
		{
			dsm.uvDimension = 2;
			for (uint32 i = 0; i < dsm.verticesCount; i++)
				ser << vec2(conv(am->mTextureCoords[0][i]));
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "uv dimensionality: " + dsm.uvDimension);
	}

	for (uint32 i = 0; i < am->mNumFaces; i++)
	{
		for (uint32 j = 0; j < indicesPerPrimitive; j++)
			ser << numeric_cast<uint32>(am->mFaces[i].mIndices[j]);
	}

	ser << mat;
	dsmPlaceholder << dsm;

	AssetHeader h = initializeAssetHeaderStruct();
	h.originalSize = dataBuffer.size();
	if (dsm.skeletonName)
		h.dependenciesCount++;
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;

	cage::MemoryBuffer compressed = detail::compress(dataBuffer);
	h.compressedSize = compressed.size();

	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(&h, sizeof(h));
	if (dsm.skeletonName)
		f->write(&dsm.skeletonName, sizeof(uint32));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(&dsm.textureNames[i], sizeof(uint32));
	f->write(compressed.data(), compressed.size());
	f->close();
}
