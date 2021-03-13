#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/color.h>
#include <cage-core/mesh.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-engine/shaderConventions.h>

#include "utility/assimp.h"

#include <vector>

vec2 convertSpecularToSpecial(const vec3 &spec);

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
			CAGE_LOG_THROW(cage::string("requested feature: ") + name);
			CAGE_THROW_ERROR(Exception, "requested feature not available");
		}
		if (requested)
		{
			flags |= idx;
			CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "feature requested: " + name);
		}
	}

	void loadTextureCage(const string &pathBase, ModelHeader &dsm, Ini *ini, const string &type, uint32 usage)
	{
		CAGE_ASSERT(usage < MaxTexturesCountPerMaterial);
		string n = ini->getString("textures", type);
		if (n.empty())
			return;
		n = convertAssetPath(n, pathBase);
		dsm.textureNames[usage] = HashString(n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + type + ", usage " + usage);
	}

	bool loadTextureAssimp(aiMaterial *m, ModelHeader &dsm, aiTextureType tt, uint32 usage)
	{
		CAGE_ASSERT(usage < MaxTexturesCountPerMaterial);
		uint32 texCount = m->GetTextureCount(tt);
		if (texCount == 0)
			return false;
		if (texCount > 1)
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "material has multiple (" + texCount + ") textures of type " + (uint32)tt + ", usage " + usage);
		aiString texAsName;
		m->GetTexture(tt, 0, &texAsName, nullptr, nullptr, nullptr, nullptr, nullptr);
		string n = texAsName.C_Str();
		if (isPattern(n, "//", "", ""))
			n = string() + "./" + subString(n, 2, cage::m);
		n = convertAssetPath(n);
		dsm.textureNames[usage] = HashString(n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture '" + n + "' (" + dsm.textureNames[usage] + ") of type " + (uint32)tt + ", usage " + usage);
		return true;
	}

	void printMaterial(const ModelHeader &dsm, const ModelHeader::MaterialData &mat)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "albedoBase: " + mat.albedoBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "albedoMult: " + mat.albedoMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "specialBase: " + mat.specialBase);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "specialMult: " + mat.specialMult);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "translucent: " + any(dsm.renderFlags & ModelRenderFlags::Translucent));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "lighting: " + any(dsm.renderFlags & ModelRenderFlags::Lighting));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "two sides: " + any(dsm.renderFlags & ModelRenderFlags::TwoSided));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "depth test: " + any(dsm.renderFlags & ModelRenderFlags::DepthTest));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "depth write: " + any(dsm.renderFlags & ModelRenderFlags::DepthWrite));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "velocity write: " + any(dsm.renderFlags & ModelRenderFlags::VelocityWrite));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "shadow cast: " + any(dsm.renderFlags & ModelRenderFlags::ShadowCast));
		for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		{
			if (dsm.textureNames[i] == 0)
				continue;
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture[" + i + "]: " + dsm.textureNames[i]);
		}
	}

	void loadMaterialCage(ModelHeader &dsm, ModelHeader::MaterialData &mat, const string &path)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using cage (.cpm) material");

		writeLine(string("use = ") + path);

		Holder<Ini> ini = newIni();
		ini->importFile(pathJoin(inputDirectory, path));

		mat.albedoBase = vec4(
			colorGammaToLinear(vec3::parse(ini->getString("base", "albedo", "0, 0, 0"))) * ini->getFloat("base", "intensity", 1),
			0 // ini->getFloat("base", "opacity", 0) // todo temporarily disabled to detect incompatibility
		);

		mat.specialBase = vec4(
			ini->getFloat("base", "roughness", 0),
			ini->getFloat("base", "metallic", 0),
			ini->getFloat("base", "emission", 0),
			0 // ini->getFloat("base", "mask", 0) // todo temporarily disabled to detect incompatibility
		);

		mat.albedoMult = vec4(
			colorGammaToLinear(vec3::parse(ini->getString("mult", "albedo", "1, 1, 1"))) * ini->getFloat("mult", "intensity", 1),
			ini->getFloat("mult", "opacity", 1)
		);

		mat.specialMult = vec4(
			ini->getFloat("mult", "roughness", 1),
			ini->getFloat("mult", "metallic", 1),
			ini->getFloat("mult", "emission", 1),
			ini->getFloat("mult", "mask", 1)
		);

		const string pathBase = pathExtractDirectory(path);
		loadTextureCage(pathBase, dsm, ini.get(), "albedo", CAGE_SHADER_TEXTURE_ALBEDO);
		loadTextureCage(pathBase, dsm, ini.get(), "special", CAGE_SHADER_TEXTURE_SPECIAL);
		loadTextureCage(pathBase, dsm, ini.get(), "normal", CAGE_SHADER_TEXTURE_NORMAL);

		for (const string &n : ini->items("flags"))
		{
			string v = ini->getString("flags", n);
			if (v == "twoSided")
			{
				dsm.renderFlags |= ModelRenderFlags::TwoSided;
				continue;
			}
			if (v == "translucent")
			{
				dsm.renderFlags |= ModelRenderFlags::Translucent;
				continue;
			}
			if (v == "noDepthTest")
			{
				dsm.renderFlags &= ~ModelRenderFlags::DepthTest;
				continue;
			}
			if (v == "noDepthWrite")
			{
				dsm.renderFlags &= ~ModelRenderFlags::DepthWrite;
				continue;
			}
			if (v == "noVelocityWrite")
			{
				dsm.renderFlags &= ~ModelRenderFlags::VelocityWrite;
				continue;
			}
			if (v == "noLighting")
			{
				dsm.renderFlags &= ~ModelRenderFlags::Lighting;
				continue;
			}
			if (v == "noShadowCast")
			{
				dsm.renderFlags &= ~ModelRenderFlags::ShadowCast;
				continue;
			}
			CAGE_LOG_THROW(stringizer() + "specified flag: '" + v + "'");
			CAGE_THROW_ERROR(Exception, "unknown material flag");
		}

		ini->checkUnused();
	}

	void loadMaterialAssimp(const aiScene *scene, const aiMesh *am, ModelHeader &dsm, ModelHeader::MaterialData &mat)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "converting assimp material");

		CAGE_ASSERT(am->mMaterialIndex < scene->mNumMaterials);
		aiMaterial *m = scene->mMaterials[am->mMaterialIndex];
		if (!m)
			CAGE_THROW_ERROR(Exception, "material is null");

		aiString matName;
		m->Get(AI_MATKEY_NAME, matName);
		if (matName.length > 0)
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "material name: '" + matName.C_Str() + "'");

		// opacity
		real opacity = 1;
		m->Get(AI_MATKEY_OPACITY, opacity.value);
		CAGE_LOG(SeverityEnum::Info, "material", stringizer() + "assimp loaded opacity: " + opacity);
		if (opacity > 0 && opacity < 1)
		{
			CAGE_LOG(SeverityEnum::Info, "material", "enabling translucent flag due to opacity");
			dsm.renderFlags |= ModelRenderFlags::Translucent;
		}

		// albedo
		if (loadTextureAssimp(m, dsm, aiTextureType_DIFFUSE, CAGE_SHADER_TEXTURE_ALBEDO))
		{
			int flg = 0;
			m->Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), flg);
			if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha && opacity > 0)
			{
				CAGE_LOG(SeverityEnum::Info, "material", "enabling translucent flag due to texture flag");
				dsm.renderFlags |= ModelRenderFlags::Translucent;
			}
		}
		else
		{
			aiColor3D color = aiColor3D(1);
			m->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mat.albedoBase = vec4(colorGammaToLinear(conv(color)), mat.albedoBase[3]);
		}

		if (opacity > 0)
			mat.albedoMult[3] = opacity;

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
				dsm.renderFlags |= ModelRenderFlags::TwoSided;
		}
	}

	void loadMaterial(const aiScene *scene, const aiMesh *am, ModelHeader &dsm, ModelHeader::MaterialData &mat)
	{
		string path = properties("material");
		if (!path.empty())
		{
			path = pathJoin(pathExtractDirectory(inputFile), path);
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

	void validateFlags(const ModelHeader &dsm, const ModelDataFlags flags, const ModelHeader::MaterialData &mat)
	{
		{
			uint32 texCount = 0;
			for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				texCount += dsm.textureNames[i] == 0 ? 0 : 1;
			if (texCount && none(flags & (ModelDataFlags::Uvs2 | ModelDataFlags::Uvs3)))
				CAGE_THROW_ERROR(Exception, "material has a texture and no uvs");
		}

		if (any(flags & ModelDataFlags::Tangents) && none(flags & ModelDataFlags::Uvs2))
			CAGE_THROW_ERROR(Exception, "tangents are exported, but uvs are missing");
		if (any(flags & ModelDataFlags::Tangents) && none(flags & ModelDataFlags::Normals))
			CAGE_THROW_ERROR(Exception, "tangents are exported, but normals are missing");

		if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && none(flags & ModelDataFlags::Normals))
			CAGE_THROW_ERROR(Exception, "model uses normal map texture but has no normals");
		if (dsm.textureNames[CAGE_SHADER_TEXTURE_NORMAL] != 0 && none(flags & ModelDataFlags::Tangents))
			CAGE_THROW_ERROR(Exception, "model uses normal map texture but has no tangents");
	}

	void loadSkeletonName(ModelHeader &dsm, const ModelDataFlags flags)
	{
		string n = properties("skeleton");
		if (none(flags & ModelDataFlags::Bones))
		{
			if (!n.empty())
				CAGE_THROW_ERROR(Exception, "cannot override skeleton for a model that has no bones");
			return;
		}
		if (n.empty())
			n = inputFile + ";skeleton";
		else
			n = pathJoin(pathExtractDirectory(inputName), n);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "using skeleton name: '" + n + "'");
		dsm.skeletonName = HashString(n.c_str());
		writeLine(string("ref = ") + n);
	}

	vec3 fixUnitVector(vec3 n, Exception::StringLiteral name)
	{
		if (abs(lengthSquared(n) - 1) > 1e-3)
		{
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "fixing denormalized " + name.str + ": " + n);
			n = normalize(n);
		}
		if (!n.valid())
		{
			static bool passInvalid = toBool(properties("passInvalidNormals"));
			if (passInvalid)
			{
				CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "pass invalid " + name.str + ": " + n);
				n = vec3();
			}
			else
				CAGE_THROW_ERROR(Exception, name);
		}
		return n;
	}

	uint32 convertPrimitiveType(int primitiveTypes)
	{
		switch (primitiveTypes)
		{
		case aiPrimitiveType_POINT: return 1;
		case aiPrimitiveType_LINE: return 2;
		case aiPrimitiveType_TRIANGLE: return 3;
		default: CAGE_THROW_ERROR(Exception, "model has invalid primitive type");
		}
	}

	uint32 computeVertexSize(ModelDataFlags flags)
	{
		uint32 p = sizeof(float) * 3;
		uint32 u2 = sizeof(float) * (int)any(flags & ModelDataFlags::Uvs2) * 2;
		uint32 u3 = sizeof(float) * (int)any(flags & ModelDataFlags::Uvs3) * 3;
		uint32 n = sizeof(float) * (int)any(flags & ModelDataFlags::Normals) * 3;
		uint32 t = sizeof(float) * (int)any(flags & ModelDataFlags::Tangents) * 6;
		uint32 b = (int)any(flags & ModelDataFlags::Bones) * (sizeof(uint16) + sizeof(float)) * 4;
		return p + u2 + u3 + n + t + b;
	}

	aabb enlarge(const aabb &box)
	{
		vec3 c = box.center();
		vec3 a = box.a - c;
		vec3 b = box.b - c;
		real s = 1.1;
		return aabb(a * s + c, b * s + c);
	}
}

void processModel()
{
	Holder<AssimpContext> context;
	{
		uint32 addFlags = 0;
		if (toBool(properties("normals")))
			addFlags |= aiProcess_GenSmoothNormals;
		if (toBool(properties("tangents")))
			addFlags |= aiProcess_CalcTangentSpace;
		context = newAssimpContext(addFlags, 0);
	}
	const aiScene *scene = context->getScene();
	const aiMesh *am = scene->mMeshes[context->selectModel()];

	if (am->GetNumUVChannels() > 1)
		CAGE_LOG(SeverityEnum::Warning, logComponentName, "multiple uv channels are not supported - using only the first");

	ModelHeader dsm;
	ModelDataFlags flags = ModelDataFlags::None;
	memset(&dsm, 0, sizeof(dsm));
	dsm.materialSize = sizeof(ModelHeader::MaterialData);
	dsm.renderFlags = ModelRenderFlags::DepthTest | ModelRenderFlags::DepthWrite | ModelRenderFlags::VelocityWrite | ModelRenderFlags::Lighting | ModelRenderFlags::ShadowCast;

	const uint32 indicesPerPrimitive = convertPrimitiveType(am->mPrimitiveTypes);
	const uint32 verticesCount = am->mNumVertices;
	const uint32 indicesCount = am->mNumFaces * indicesPerPrimitive;

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "indices per primitive: " + indicesPerPrimitive);
	CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "vertices count: " + verticesCount);
	CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "indices count: " + indicesCount);

	setFlags(flags, ModelDataFlags::Uvs2, am->GetNumUVChannels() > 0, "uvs");
	setFlags(flags, ModelDataFlags::Normals, am->HasNormals(), "normals");
	setFlags(flags, ModelDataFlags::Tangents, am->HasTangentsAndBitangents(), "tangents");
	setFlags(flags, ModelDataFlags::Bones, am->HasBones(), "bones");

	if (am->mNumUVComponents[0] == 3)
	{
		flags &= ~ModelDataFlags::Uvs2;
		flags |= ModelDataFlags::Uvs3;
	}

	if (any(flags & ModelDataFlags::Uvs2))
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using 2D uvs");
	if (any(flags & ModelDataFlags::Uvs3))
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using 3D uvs");

	loadSkeletonName(dsm, flags);

	dsm.instancesLimitHint = toUint32(properties("instancesLimit"));

	ModelHeader::MaterialData mat;
	loadMaterial(scene, am, dsm, mat);
	printMaterial(dsm, mat);
	validateFlags(dsm, flags, mat);

	dsm.box = aabb();
	const mat3 axes = axesMatrix();
	const mat3 axesScale = axesScaleMatrix();

	Holder<Mesh> poly = newMesh();
	switch (indicesPerPrimitive)
	{
	case 1: poly->type(MeshTypeEnum::Points); break;
	case 2: poly->type(MeshTypeEnum::Lines); break;
	case 3: poly->type(MeshTypeEnum::Triangles); break;
	default: CAGE_THROW_CRITICAL(Exception, "invalid mesh type enum");
	}

	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying vertex positions");;
		std::vector<vec3> ps;
		ps.reserve(verticesCount);
		for (uint32 i = 0; i < verticesCount; i++)
		{
			vec3 p = axesScale * conv(am->mVertices[i]);
			dsm.box += aabb(p);
			ps.push_back(p);
		}
		poly->positions(ps);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "bounding box: " + dsm.box);
	}

	if (any(flags & ModelDataFlags::Normals))
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying normals");;
		std::vector<vec3> ps;
		ps.reserve(verticesCount);
		for (uint32 i = 0; i < verticesCount; i++)
		{
			vec3 n = axes * conv(am->mNormals[i]);
			ps.push_back(fixUnitVector(n, "normal"));
		}
		poly->normals(ps);
	}

	if (any(flags & ModelDataFlags::Tangents))
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying tangents");;
		std::vector<vec3> ts, bs;
		ts.reserve(verticesCount);
		bs.reserve(verticesCount);
		for (uint32 i = 0; i < verticesCount; i++)
		{
			vec3 n = axes * conv(am->mTangents[i]);
			ts.push_back(fixUnitVector(n, "tangent"));
		}
		for (uint32 i = 0; i < verticesCount; i++)
		{
			vec3 n = axes * conv(am->mBitangents[i]);
			bs.push_back(fixUnitVector(n, "bitangent"));
		}
		poly->tangents(ts);
		poly->bitangents(bs);
	}

	if (any(flags & ModelDataFlags::Bones))
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying bones");;
		CAGE_ASSERT(am->mNumBones > 0);
		Holder<AssimpSkeleton> skeleton = context->skeleton();
		dsm.skeletonBones = skeleton->bonesCount();
		std::vector<ivec4> boneIndices;
		boneIndices.resize(verticesCount, ivec4((sint32)m));
		std::vector<vec4> boneWeights;
		boneWeights.resize(verticesCount);
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
					if (boneIndices[w->mVertexId][i] == m)
					{
						boneIndices[w->mVertexId][i] = boneId;
						boneWeights[w->mVertexId][i] = w->mWeight;
						ok = true;
						break;
					}
				}
				CAGE_ASSERT(ok);
			}
		}
		// validate
		uint32 maxBoneId = 0;
		for (uint32 i = 0; i < verticesCount; i++)
		{
			real sum = 0;
			for (uint32 j = 0; j < 4; j++)
			{
				if (boneIndices[i][j] == m)
				{
					CAGE_ASSERT(boneWeights[i][j] == 0);
					boneIndices[i][j] = 0; // prevent shader from accessing invalid memory
				}
				sum += boneWeights[i][j];
				maxBoneId = max(maxBoneId, boneIndices[i][j] + 1u);
			}
			// renormalize weights
			if (cage::abs(sum - 1) > 1e-3 && sum > 1e-3)
			{
				const real f = 1 / sum;
				CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "renormalizing bone weights for " + i + "th vertex by factor " + f);
				for (uint32 j = 0; j < 4; j++)
					boneWeights[i][j] *= f;
			}
		}
		CAGE_ASSERT(maxBoneId <= dsm.skeletonBones);
		poly->boneIndices(boneIndices);
		poly->boneWeights(boneWeights);
	}

	if (any(flags & ModelDataFlags::Uvs3))
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying 3D uvs");;
		std::vector<vec3> ts;
		ts.reserve(verticesCount);
		for (uint32 i = 0; i < verticesCount; i++)
			ts.push_back(conv(am->mTextureCoords[0][i]));
		poly->uvs3(ts);
	}

	if (any(flags & ModelDataFlags::Uvs2))
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying uvs");;
		std::vector<vec2> ts;
		ts.reserve(verticesCount);
		for (uint32 i = 0; i < verticesCount; i++)
			ts.push_back(vec2(conv(am->mTextureCoords[0][i])));
		poly->uvs(ts);
	}

	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "copying indices");;
		std::vector<uint32> inds;
		inds.reserve(indicesCount);
		for (uint32 i = 0; i < am->mNumFaces; i++)
		{
			for (uint32 j = 0; j < indicesPerPrimitive; j++)
				inds.push_back(numeric_cast<uint32>(am->mFaces[i].mIndices[j]));
		}
		poly->indices(inds);
	}

	if (any(flags & ModelDataFlags::Bones))
	{
		// replay all animations and enlarge the bounding box to contain them
		Holder<SkeletonRig> rig = context->skeletonRig();
		for (uint32 i = 0; i < scene->mNumAnimations; i++)
		{
			Holder<SkeletalAnimation> ani = context->animation(i);
			for (real t = 0; t <= 1; t += 0.02) // sample the animation at 50 positions
			{
				Holder<Mesh> tmp = poly->copy();
				animateMesh(+rig, +ani, t, +tmp);
				//tmp->exportObjFile({}, pathReplaceInvalidCharacters(stringizer() + "skellies/" + i + "/" + t + "/" + inputName + ".obj", "_", true));
				aabb box = tmp->boundingBox();
				box = enlarge(box);
				dsm.box += box;
			}
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "enlarged bounding box: " + dsm.box);
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, "serializing");;
	AssetHeader h = initializeAssetHeader();
	if (dsm.skeletonName)
		h.dependenciesCount++;
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			h.dependenciesCount++;

	cage::MemoryBuffer buffer;
	Serializer ser(buffer);
	ser << dsm;
	ser << mat;
	ser.write(poly->serialize());
	h.originalSize = buffer.size();
	Holder<PointerRange<char>> compressed = compress(buffer);
	h.compressedSize = compressed.size();

	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	if (dsm.skeletonName)
		f->write(bufferView(dsm.skeletonName));
	for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
		if (dsm.textureNames[i])
			f->write(bufferView(dsm.textureNames[i]));
	f->write(compressed);
	f->close();
}
