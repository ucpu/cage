#include "utility/assimp.h"
#include <cage-core/hashString.h>
#include <cage-core/configIni.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/opengl.h>

vec2 convertSpecularToSpecial(const vec3 &spec);

namespace
{
	void setFlags(meshDataFlags &flags, meshDataFlags idx, bool available, const char *name)
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

	void loadTextureCage(const string &pathBase, renderMeshHeader &dsm, configIni *ini, const string &type, uint32 usage)
	{
		CAGE_ASSERT_RUNTIME(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, type);
		string n = ini->getString("textures", type);
		if (n.empty())
			return;
		n = pathJoin(pathBase, n);
		dsm.textureNames[usage] = hashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + type + ", usage " + usage);
	}

	bool loadTextureAssimp(aiMaterial *m, renderMeshHeader &dsm, aiTextureType tt, uint32 usage)
	{
		CAGE_ASSERT_RUNTIME(usage < MaxTexturesCountPerMaterial, usage, MaxTexturesCountPerMaterial, tt);
		uint32 texCount = m->GetTextureCount(tt);
		if (texCount == 0)
			return false;
		if (texCount > 1)
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "material has multiple (" + texCount + ") textures of type " + (uint32)tt + ", usage " + usage);
		aiString texAsName;
		m->GetTexture(tt, 0, &texAsName, nullptr, nullptr, nullptr, nullptr, nullptr);
		cage::string tn = texAsName.C_Str();
		if (tn.isPattern("//", "", ""))
			tn = string() + "./" + tn.subString(2, cage::m);
		cage::string n = pathJoin(pathExtractPath(inputName), tn);
		dsm.textureNames[usage] = hashString(n.c_str());
		writeLine(string("ref = ") + n);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + (uint32)tt + ", usage " + usage);
		return true;
	}

	void printMaterial(const renderMeshHeader &dsm, const renderMeshHeader::materialData &mat)
	{
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "specialBase: " + mat.specialBase);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "specialMult: " + mat.specialMult);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "opacity texture: " + ((dsm.renderFlags & meshRenderFlags::OpacityTexture) == meshRenderFlags::OpacityTexture));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "transparency: " + ((dsm.renderFlags & meshRenderFlags::Transparency) == meshRenderFlags::Transparency));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "translucency: " + ((dsm.renderFlags & meshRenderFlags::Translucency) == meshRenderFlags::Translucency));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "lighting: " + ((dsm.renderFlags & meshRenderFlags::Lighting) == meshRenderFlags::Lighting));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "two sides: " + ((dsm.renderFlags & meshRenderFlags::TwoSided) == meshRenderFlags::TwoSided));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "depth test: " + ((dsm.renderFlags & meshRenderFlags::DepthTest) == meshRenderFlags::DepthTest));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "depth write: " + ((dsm.renderFlags & meshRenderFlags::DepthWrite) == meshRenderFlags::DepthWrite));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "velocity write: " + ((dsm.renderFlags & meshRenderFlags::VelocityWrite) == meshRenderFlags::VelocityWrite));
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "shadow cast: " + ((dsm.renderFlags & meshRenderFlags::ShadowCast) == meshRenderFlags::ShadowCast));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i] == 0)
				continue;
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void loadMaterialCage(renderMeshHeader &dsm, renderMeshHeader::materialData &mat, string path)
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "using cage (.cpm) material");

		writeLine(string("use = ") + path);

		holder<configIni> ini = newConfigIni();
		ini->load(pathJoin(inputDirectory, path));

		mat.albedoBase = vec4(
			vec3::parse(ini->getString("base", "albedo", "0, 0, 0")),
			ini->getString("base", "opacity", "1").toFloat()
		);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.renderFlags |= meshRenderFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.renderFlags |= meshRenderFlags::Translucency;

		mat.specialBase = vec4(
			ini->getString("base", "roughness", "0").toFloat(),
			ini->getString("base", "metalness", "0").toFloat(),
			ini->getString("base", "emissive", "0").toFloat(),
			ini->getString("base", "mask", "0").toFloat()
		);

		mat.albedoMult = vec4(vec3::parse(ini->getString("mult", "albedo", "1, 1, 1")), 1);

		mat.specialMult = vec4(
			ini->getString("mult", "roughness", "1").toFloat(),
			ini->getString("mult", "metalness", "1").toFloat(),
			ini->getString("mult", "emissive", "1").toFloat(),
			ini->getString("mult", "mask", "1").toFloat()
		);

		string pathBase = pathExtractPath(path);
		loadTextureCage(pathBase, dsm, ini.get(), "albedo", CAGE_SHADER_TEXTURE_ALBEDO);
		loadTextureCage(pathBase, dsm, ini.get(), "special", CAGE_SHADER_TEXTURE_SPECIAL);
		loadTextureCage(pathBase, dsm, ini.get(), "normal", CAGE_SHADER_TEXTURE_NORMAL);

		for (const string &n : ini->items("flags"))
		{
			string v = ini->get("flags", n);
			if (v == "opacity-texture")
			{
				dsm.renderFlags |= meshRenderFlags::OpacityTexture;
				continue;
			}
			if (v == "two-sided")
			{
				dsm.renderFlags |= meshRenderFlags::TwoSided;
				continue;
			}
			if (v == "transparency")
			{
				dsm.renderFlags |= meshRenderFlags::Transparency;
				continue;
			}
			if (v == "translucency")
			{
				dsm.renderFlags |= meshRenderFlags::Translucency;
				continue;
			}
			if (v == "no-depth-test")
			{
				dsm.renderFlags &= ~meshRenderFlags::DepthTest;
				continue;
			}
			if (v == "no-depth-write")
			{
				dsm.renderFlags &= ~meshRenderFlags::DepthWrite;
				continue;
			}
			if (v == "no-velocity-write")
			{
				dsm.renderFlags &= ~meshRenderFlags::VelocityWrite;
				continue;
			}
			if (v == "no-lighting")
			{
				dsm.renderFlags &= ~meshRenderFlags::Lighting;
				continue;
			}
			if (v == "no-shadow-cast")
			{
				dsm.renderFlags &= ~meshRenderFlags::ShadowCast;
				continue;
			}
			CAGE_LOG(severityEnum::Note, "exception", string() + "specified flag: '" + v + "'");
			CAGE_THROW_ERROR(exception, "unknown material flag");
		}
	}

	void loadMaterialAssimp(const aiScene *scene, const aiMesh *am, renderMeshHeader &dsm, renderMeshHeader::materialData &mat)
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

		// opacity
		m->Get(AI_MATKEY_OPACITY, mat.albedoBase[3].value);
		if (mat.albedoBase[3] <= 1e-7)
			dsm.renderFlags |= meshRenderFlags::Transparency;
		else if (mat.albedoBase[3] < 1)
			dsm.renderFlags |= meshRenderFlags::Translucency;

		// albedo
		if (loadTextureAssimp(m, dsm, aiTextureType_DIFFUSE, CAGE_SHADER_TEXTURE_ALBEDO))
		{
			int flg = 0;
			m->Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), flg);
			if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha || mat.albedoBase[3] <= 1e-7)
			{
				dsm.renderFlags |= meshRenderFlags::OpacityTexture;
				if ((dsm.renderFlags & meshRenderFlags::Transparency) == meshRenderFlags::None)
					dsm.renderFlags |= meshRenderFlags::Translucency;
			}
		}
		else
		{
			aiColor3D color = aiColor3D(1);
			m->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mat.albedoBase = vec4(conv(color), mat.albedoBase[3]);

			if (mat.albedoBase[3] < 1e-7)
				dsm.renderFlags |= meshRenderFlags::Transparency;
			else if (mat.albedoBase[3] < 1 - 1e-7)
				dsm.renderFlags |= meshRenderFlags::Translucency;
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
				dsm.renderFlags |= meshRenderFlags::TwoSided;
		}
	}

	void loadMaterial(const aiScene *scene, const aiMesh *am, renderMeshHeader &dsm, renderMeshHeader::materialData &mat)
	{
		string path = properties("override_material");
		if (!path.empty())
		{
			path = pathJoin(pathExtractPath(inputFile), path);
			if (!pathIsFile(pathJoin(inputDirectory, path)))
				CAGE_THROW_ERROR(exception, "overriden material path does not exist");
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

		CAGE_LOG(severityEnum::Info, logComponentName, string() + "looking for implicit cage material at '" + path + "'");

		if (pathIsFile(pathJoin(inputDirectory, path)))
		{
			loadMaterialCage(dsm, mat, path);
			return;
		}

		loadMaterialAssimp(scene, am, dsm, mat);
	}

	void validateFlags(const renderMeshHeader &dsm, const renderMeshHeader::materialData &mat)
	{
		if ((dsm.renderFlags & meshRenderFlags::OpacityTexture) == meshRenderFlags::OpacityTexture && (dsm.renderFlags & (meshRenderFlags::Translucency | meshRenderFlags::Transparency)) == meshRenderFlags::None)
			CAGE_THROW_ERROR(exception, "material flags contains opacity texture, but neither translucency nor transparency is set");
		if ((dsm.renderFlags & meshRenderFlags::Translucency) == meshRenderFlags::Translucency && (dsm.renderFlags & meshRenderFlags::Transparency) == meshRenderFlags::Transparency)
			CAGE_THROW_ERROR(exception, "material flags transparency and translucency are mutually exclusive");
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && (dsm.flags & meshDataFlags::Uvs) == meshDataFlags::None)
				CAGE_THROW_ERROR(exception, "material has textures, but uvs are missing");
		}
		if ((dsm.flags & meshDataFlags::Tangents) == meshDataFlags::Tangents && (dsm.flags & meshDataFlags::Uvs) == meshDataFlags::None)
			CAGE_THROW_ERROR(exception, "tangents are exported, but uvs are missing");
		if ((dsm.flags & meshDataFlags::Tangents) == meshDataFlags::Tangents && (dsm.flags & meshDataFlags::Normals) == meshDataFlags::None)
			CAGE_THROW_ERROR(exception, "tangents are exported, but normals are missing");
	}

	void loadSkeletonName(renderMeshHeader &dsm)
	{
		string n = properties("override_skeleton");
		if (!dsm.bones())
		{
			if (!n.empty())
				CAGE_THROW_ERROR(exception, "cannot override skeleton for a mesh that has no bones");
			return;
		}
		if (n.empty())
			n = inputFile + ";skeleton";
		else
			n = pathJoin(pathExtractPath(inputName), n);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "using skeleton name: '" + n + "'");
		dsm.skeletonName = hashString(n.c_str());
		writeLine(string("ref = ") + n);
	}
}

void processMesh()
{
	holder<assimpContextClass> context;
	{
		uint32 addFlags = 0;
		if (properties("export_normal").toBool())
			addFlags |= aiProcess_GenSmoothNormals;
		if (properties("export_tangent").toBool())
			addFlags |= aiProcess_CalcTangentSpace;
		context = newAssimpContext(addFlags, 0);
	}
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectMesh()];

	if (am->GetNumUVChannels() > 1)
		CAGE_LOG(severityEnum::Warning, logComponentName, "multiple uv channels are not supported - using only the first");

	renderMeshHeader dsm;
	memset(&dsm, 0, sizeof(dsm));
	dsm.materialSize = sizeof(renderMeshHeader::materialData);
	dsm.renderFlags = meshRenderFlags::DepthTest | meshRenderFlags::DepthWrite | meshRenderFlags::VelocityWrite | meshRenderFlags::Lighting | meshRenderFlags::ShadowCast;

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

	setFlags(dsm.flags, meshDataFlags::Uvs, am->GetNumUVChannels() > 0, "export_uv");
	setFlags(dsm.flags, meshDataFlags::Normals, am->HasNormals(), "export_normal");
	setFlags(dsm.flags, meshDataFlags::Tangents, am->HasTangentsAndBitangents(), "export_tangent");
	setFlags(dsm.flags, meshDataFlags::Bones, am->HasBones(), "export_bones");

	loadSkeletonName(dsm);

	dsm.instancesLimitHint = properties("instances_limit").toUint32();

	renderMeshHeader::materialData mat;
	memset(&mat, 0, sizeof(mat));
	mat.albedoBase = vec4(0, 0, 0, 1);
	mat.specialBase = vec4(0, 0, 0, 0);
	mat.albedoMult = vec4(1, 1, 1, 1);
	mat.specialMult = vec4(1, 1, 1, 1);

	loadMaterial(scene, am, dsm, mat);
	printMaterial(dsm, mat);
	validateFlags(dsm, mat);

	cage::memoryBuffer dataBuffer;
	dataBuffer.reserve(dsm.vertexSize() * dsm.verticesCount);
	cage::serializer ser(dataBuffer);
	cage::serializer dsmPlaceholder = ser.placeholder(sizeof(dsm));

	dsm.box = aabb();
	mat3 axes = axesMatrix();
	mat3 axesScale = axesScaleMatrix();
	for (uint32 i = 0; i < dsm.verticesCount; i++)
	{
		vec3 p = axesScale * conv(am->mVertices[i]);
		ser << p;
		dsm.box += aabb(p);
	}
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "bounding box: " + dsm.box);

	if (dsm.normals())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
			ser << axes * conv(am->mNormals[i]);
	}

	if (dsm.tangents())
	{
		for (uint32 i = 0; i < dsm.verticesCount; i++)
			ser << axes * conv(am->mTangents[i]);
		for (uint32 i = 0; i < dsm.verticesCount; i++)
			ser << axes * conv(am->mBitangents[i]);
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
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "enlarged bounding box: " + dsm.box);
		}
		CAGE_ASSERT_RUNTIME(am->mNumBones > 0);
		holder<assimpSkeletonClass> skeleton = context->skeleton();
		dsm.skeletonBones = skeleton->bonesCount();
		serializer ser2 = ser.placeholder((sizeof(uint16) + sizeof(float)) * 4 * dsm.verticesCount);
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
			CAGE_ASSERT_RUNTIME(bone);
			uint16 boneId = skeleton->index(bone);
			CAGE_ASSERT_RUNTIME(boneId != m);
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
				CAGE_ASSERT_RUNTIME(ok, "single vertex may not be affected by more than four bones");
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
					CAGE_ASSERT_RUNTIME(boneWeights[i * 4 + j] == 0, i, j);
					boneIndices[i * 4 + j] = 0; // prevent shader from accessing invalid memory
				}
				sum += boneWeights[i * 4 + j];
				maxBoneId = max(maxBoneId, boneIndices[i * 4 + j] + 1u);
			}
			// renormalize weights
			if (cage::abs(sum - 1) > 1e-3 && sum > 1e-3)
			{
				float f = 1 / sum;
				CAGE_LOG(severityEnum::Warning, logComponentName, string() + "renormalizing bone weights for " + i + "th vertex by factor " + f);
				for (uint32 j = 0; j < 4; j++)
					boneWeights[i * 4 + j] *= f;
			}
		}
		CAGE_ASSERT_RUNTIME(maxBoneId <= dsm.skeletonBones, maxBoneId, dsm.skeletonBones);
	}

	if (dsm.uvs())
	{
		dsm.uvDimension = 3;
		for (uint32 i = 0; i < dsm.verticesCount; i++)
			ser << conv(am->mTextureCoords[0][i]);
	}

	for (uint32 i = 0; i < am->mNumFaces; i++)
	{
		for (uint32 j = 0; j < indicesPerPrimitive; j++)
			ser << numeric_cast<uint32>(am->mFaces[i].mIndices[j]);
	}

	ser << mat;
	dsmPlaceholder << dsm;

	assetHeader h = initializeAssetHeaderStruct();
	h.originalSize = dataBuffer.size();
	if (dsm.skeletonName)
		h.dependenciesCount++;
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;

	cage::memoryBuffer compressed = detail::compress(dataBuffer);
	h.compressedSize = compressed.size();

	holder<fileHandle> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	if (dsm.skeletonName)
		f->write(&dsm.skeletonName, sizeof(uint32));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(&dsm.textureNames[i], sizeof(uint32));
	f->write(compressed.data(), compressed.size());
	f->close();
}
