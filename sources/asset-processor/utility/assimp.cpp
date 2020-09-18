#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

#include "assimp.h"

#include <set>
#include <map>
#include <vector>

namespace
{
	class AssimpSkeletonImpl : public AssimpSkeleton
	{
	public:
		AssimpSkeletonImpl(const aiScene *scene)
		{
			// find nodes and parents
			{
				std::map<aiString, aiNode*, CmpAiStr> names;
				// find names
				traverseNodes1(names, scene->mRootNode);
				std::set<aiString, CmpAiStr> interested;
				// find interested
				for (uint32 mi = 0; mi < scene->mNumMeshes; mi++)
				{
					aiMesh *m = scene->mMeshes[mi];
					for (uint32 bi = 0; bi < m->mNumBones; bi++)
					{
						aiBone *b = m->mBones[bi];
						CAGE_ASSERT(names.count(b->mName) == 1);
						aiNode *n = names[b->mName];
						while (n && interested.count(n->mName) == 0)
						{
							interested.insert(n->mName);
							n = n->mParent;
						}
					}
				}
				// find nodes
				traverseNodes2(interested, scene->mRootNode, m);
			}
			// update indices
			{
				uint16 i = 0;
				for (auto n : nodes)
					indices[n->mName] = i++;
			}
			// update bones
			bones.resize(nodes.size(), nullptr);
			for (uint32 mi = 0; mi < scene->mNumMeshes; mi++)
			{
				aiMesh *m = scene->mMeshes[mi];
				for (uint32 bi = 0; bi < m->mNumBones; bi++)
				{
					aiBone *b = m->mBones[bi];
					CAGE_ASSERT(indices.count(b->mName));
					uint16 idx = indices[b->mName];
					if (bones[idx])
					{
						CAGE_ASSERT(conv(bones[idx]->mOffsetMatrix) == conv(b->mOffsetMatrix));
					}
					else
						bones[idx] = b;
				}
			}
#ifdef CAGE_ASSERT_ENABLED
			validate();
#endif // CAGE_ASSERT_ENABLED
		}

		void validate()
		{
			uint32 num = numeric_cast<uint32>(bones.size());
			CAGE_ASSERT(num == nodes.size());
			CAGE_ASSERT(num == parents.size());
			for (uint32 i = 0; i < num; i++)
			{
				if (nodes[i]->mParent)
				{
					CAGE_ASSERT(parents[i] < i);
					CAGE_ASSERT(parent(nodes[i]) == nodes[parents[i]]);
				}
				else
				{
					CAGE_ASSERT(parents[i] == m);
				}
				if (bones[i])
				{
					CAGE_ASSERT(bones[i]->mName == nodes[i]->mName);
				}
				if (nodes[i]->mName.length)
				{
					CAGE_ASSERT(indices.count(nodes[i]->mName) == 1);
					CAGE_ASSERT(indices[nodes[i]->mName] == i);
				}
			}
		}

		void traverseNodes1(std::map<aiString, aiNode*, CmpAiStr> &names, aiNode *n)
		{
			if (n->mName.length)
			{
				if (names.count(n->mName))
				{ // make the name unique
					n->mName = string(stringizer() + n->mName.C_Str() + "_" + n).c_str();
					CAGE_LOG_DEBUG(SeverityEnum::Warning, logComponentName, stringizer() + "renamed a node: '" + n->mName.C_Str() + "'");
				}
				names[n->mName] = n;
			}
			for (uint32 i = 0; i < n->mNumChildren; i++)
				traverseNodes1(names, n->mChildren[i]);
		}

		void traverseNodes2(const std::set<aiString, CmpAiStr> &interested, aiNode *n, uint16 pi)
		{
			if (interested.count(n->mName) == 0)
				return;
			nodes.push_back(n);
			parents.push_back(pi);
			pi = numeric_cast<uint16>(parents.size() - 1);
			for (uint32 i = 0; i < n->mNumChildren; i++)
				traverseNodes2(interested, n->mChildren[i], pi);
		}

		std::vector<aiBone*> bones;
		std::vector<aiNode*> nodes;
		std::vector<uint16> parents;
		std::map<aiString, uint16, CmpAiStr> indices;
	};

	class CageIoStream : public Assimp::IOStream
	{
	public:
		CageIoStream(cage::Holder<cage::File> r) : r(templates::move(r))
		{}

		virtual ~CageIoStream()
		{}

		virtual size_t Read(void *pvBuffer, size_t pSize, size_t pCount)
		{
			uint64 avail = r->size() - r->tell();
			uint64 size = pSize * pCount;
			while (size > avail)
			{
				size -= pSize;
				pCount--;
			}
			r->read({ (char*)pvBuffer, (char*)pvBuffer + size });
			return pCount;
		}

		virtual size_t Write(const void *pvBuffer, size_t pSize, size_t pCount)
		{
			CAGE_THROW_ERROR(Exception, "cageIOStream::Write is not meant for use");
		}

		virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin)
		{
			switch (pOrigin)
			{
			case aiOrigin_SET: r->seek(pOffset); break;
			case aiOrigin_CUR: r->seek(r->tell() + pOffset); break;
			case aiOrigin_END: r->seek(r->size() + pOffset); break;
			default: CAGE_THROW_CRITICAL(Exception, "cageIOStream::Seek: unknown pOrigin");
			}
			return aiReturn_SUCCESS;
		}

		virtual size_t Tell() const
		{
			return (size_t)r->tell();
		}

		virtual size_t FileSize() const
		{
			return (size_t)r->size();
		}

		virtual void Flush()
		{
			CAGE_THROW_ERROR(Exception, "cageIOStream::Flush is not meant for use");
		}

	private:
		cage::Holder<cage::File> r;
	};

	class CageIoSystem : public Assimp::IOSystem
	{
	public:
		CageIoSystem()
		{
			currentDir = pathJoin(inputDirectory, pathExtractPath(inputFile));
		}

		virtual bool Exists(const char *pFile) const
		{
			return any(pathType(pathJoin(currentDir, pFile)) & PathTypeFlags::File);
		}

		virtual char getOsSeparator() const
		{
			return '/';
		}

		virtual Assimp::IOStream *Open(const char *pFile, const char *pMode = "rb")
		{
			if (::cage::string(pMode) != "rb")
				CAGE_THROW_ERROR(Exception, "CageIoSystem::Open: only support rb mode");
			writeLine(cage::string("use = ") + pathJoin(pathExtractPath(inputFile), pFile));
			return new CageIoStream(newFile(pathJoin(currentDir, pFile), FileMode(true, false)));
		}

		virtual void Close(Assimp::IOStream *pFile)
		{
			delete (CageIoStream*)pFile;
		}

		virtual bool ComparePaths(const char *one, const char *second) const
		{
			CAGE_THROW_ERROR(Exception, "cageIOsystem::ComparePaths is not meant for use");
		}

	private:
		string currentDir;
	};

	class CageLogStream : public Assimp::LogStream
	{
	public:
		cage::SeverityEnum severity;
		CageLogStream(cage::SeverityEnum severity) : severity(severity)
		{}
		virtual void write(const char* message)
		{
			string m = message;
			if (m.isPattern("", "", "\n"))
				m = m.subString(0, m.length() - 1);
			CAGE_LOG(severity, "assimp", m);
		}
	};

	struct AssimpContextImpl : public AssimpContext, Immovable
	{
		static constexpr uint32 assimpDefaultLoadFlags =
			aiProcess_JoinIdenticalVertices |
			aiProcess_Triangulate |
			aiProcess_LimitBoneWeights |
			//aiProcess_ValidateDataStructure |
			aiProcess_ImproveCacheLocality |
			aiProcess_SortByPType |
			//aiProcess_FindInvalidData |
			aiProcess_GenUVCoords |
			aiProcess_TransformUVCoords |
			aiProcess_FindDegenerates |
			aiProcess_OptimizeGraph |
			//aiProcess_Debone | // see https://github.com/assimp/assimp/issues/2547
			//aiProcess_SplitLargeMeshes |
			0;

		static constexpr uint32 assimpBakeLoadFlags =
			aiProcess_RemoveRedundantMaterials |
			//aiProcess_FindInstances |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			0;

		explicit AssimpContextImpl(uint32 addFlags, uint32 removeFlags) : logDebug(SeverityEnum::Note), logInfo(SeverityEnum::Info), logWarn(SeverityEnum::Warning), logError(SeverityEnum::Error)
		{
#ifdef CAGE_DEBUG
			Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
#else
			Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
#endif
			Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_FILE);
			Assimp::DefaultLogger::get()->attachStream(&logDebug, Assimp::Logger::Debugging);
			Assimp::DefaultLogger::get()->attachStream(&logInfo, Assimp::Logger::Info);
			Assimp::DefaultLogger::get()->attachStream(&logWarn, Assimp::Logger::Warn);
			Assimp::DefaultLogger::get()->attachStream(&logError, Assimp::Logger::Err);

			//imp.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, true);
			imp.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 30);

			try
			{
				uint32 flags = assimpDefaultLoadFlags;
				if (string(logComponentName) != "analyze")
				{
					if (properties("bakeModel").toBool())
						flags |= assimpBakeLoadFlags;
				}
				flags |= addFlags;
				flags &= ~removeFlags;
				CAGE_LOG(SeverityEnum::Info, logComponentName, cage::stringizer() + "assimp loading flags: " + flags);

				imp.SetIOHandler(&this->ioSystem);
				if (!imp.ReadFile(pathExtractFilename(inputFile).c_str(), flags))
				{
					CAGE_LOG(SeverityEnum::Note, "exception", cage::string(imp.GetErrorString()));
					CAGE_THROW_ERROR(Exception, "assimp loading failed");
				}
				imp.SetIOHandler(nullptr);
			}
			catch (...)
			{
				imp.SetIOHandler(nullptr);
				throw;
			}

			const aiScene *scene = imp.GetScene();

			if (!scene)
				CAGE_THROW_ERROR(Exception, "scene is null");

			if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == AI_SCENE_FLAGS_INCOMPLETE)
				CAGE_THROW_ERROR(Exception, "the scene is incomplete");

			// print meshes
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "found " + scene->mNumMeshes + " meshes");
			for (uint32 i = 0; i < scene->mNumMeshes; i++)
			{
				const aiMesh *am = scene->mMeshes[i];
				string objname = am->mName.C_Str();
				string matname;
				aiString aiMatName;
				scene->mMaterials[am->mMaterialIndex]->Get(AI_MATKEY_NAME, aiMatName);
				matname = string(aiMatName.C_Str());
				string contains;
				if (am->mPrimitiveTypes & aiPrimitiveType_POINT)
					contains += "points ";
				if (am->mPrimitiveTypes & aiPrimitiveType_LINE)
					contains += "lines ";
				if (am->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)
					contains += "triangles ";
				if (am->mPrimitiveTypes & aiPrimitiveType_POLYGON)
					contains += "polygons ";
				if (am->HasBones())
					contains += "bones ";
				if (am->HasNormals())
					contains += "normals ";
				if (am->HasTextureCoords(0))
					contains += "uvs ";
				if (am->HasTangentsAndBitangents())
					contains += "tangents ";
				if (am->HasVertexColors(0))
					contains += "colors ";
				CAGE_LOG_CONTINUE(SeverityEnum::Note, logComponentName, stringizer() + "index: " + i + ", object: '" + objname + "', material: '" + matname + "', contains: " + contains);
			}

			// print animations
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "found " + scene->mNumAnimations + " animations");
			for (uint32 i = 0; i < scene->mNumAnimations; i++)
			{
				const aiAnimation *ani = scene->mAnimations[i];
				CAGE_LOG_CONTINUE(SeverityEnum::Info, logComponentName, stringizer() + "index: " + i + ", animation: '" + ani->mName.data + "', channels: " + ani->mNumChannels);
			}
		};

		~AssimpContextImpl()
		{
			//imp.FreeScene();
			//imp.SetIOHandler(nullptr);
			Assimp::DefaultLogger::get()->detatchStream(&logDebug);
			Assimp::DefaultLogger::get()->detatchStream(&logInfo);
			Assimp::DefaultLogger::get()->detatchStream(&logWarn);
			Assimp::DefaultLogger::get()->detatchStream(&logError);
			Assimp::DefaultLogger::kill();
		};

		const aiScene *getScene() const
		{
			return imp.GetScene();
		}

	private:
		CageIoSystem ioSystem;
		CageLogStream logDebug, logInfo, logWarn, logError;
		Assimp::Importer imp;
	};
}

uint16 AssimpSkeleton::bonesCount() const
{
	const AssimpSkeletonImpl *impl = (const AssimpSkeletonImpl*)this;
	CAGE_ASSERT(impl->bones.size() == impl->nodes.size());
	CAGE_ASSERT(impl->parents.size() == impl->nodes.size());
	return numeric_cast<uint16>(impl->bones.size());
}

aiNode *AssimpSkeleton::node(aiBone *bone) const
{
	return node(index(bone));
}

aiNode *AssimpSkeleton::node(uint16 index) const
{
	const AssimpSkeletonImpl *impl = (const AssimpSkeletonImpl*)this;
	if (index == m)
		return nullptr;
	return impl->nodes[index];
}

aiBone *AssimpSkeleton::bone(aiNode *node) const
{
	return bone(index(node));
}

aiBone *AssimpSkeleton::bone(uint16 index) const
{
	const AssimpSkeletonImpl *impl = (const AssimpSkeletonImpl*)this;
	if (index == m)
		return nullptr;
	return impl->bones[index];
}

uint16 AssimpSkeleton::index(const aiString &name) const
{
	const AssimpSkeletonImpl *impl = (const AssimpSkeletonImpl*)this;
	if (!name.length)
		return m;
	if (!impl->indices.count(name))
		return m;
	return impl->indices.at(name);
}

uint16 AssimpSkeleton::index(aiNode *node) const
{
	if (!node)
		return m;
	return index(node->mName);
}

uint16 AssimpSkeleton::index(aiBone *bone) const
{
	if (!bone)
		return m;
	return index(bone->mName);
}

uint16 AssimpSkeleton::parent(uint16 index) const
{
	const AssimpSkeletonImpl *impl = (const AssimpSkeletonImpl*)this;
	if (index == m)
		return m;
	return impl->parents[index];
}

aiBone *AssimpSkeleton::parent(aiBone *bone) const
{
	return this->bone(parent(index(bone)));
}

aiNode *AssimpSkeleton::parent(aiNode *node) const
{
	return this->node(parent(index(node)));
}

const aiScene *AssimpContext::getScene() const
{
	return ((AssimpContextImpl*)this)->getScene();
}

uint32 AssimpContext::selectMesh() const
{
	const aiScene *scene = getScene();
	if (scene->mNumMeshes == 1 && inputSpec.empty())
	{
		CAGE_LOG(SeverityEnum::Note, "selectMesh", "using the first mesh, because it is the only mesh available");
		return 0;
	}
	if (inputSpec.isDigitsOnly() && !inputSpec.empty())
	{
		uint32 n = inputSpec.toUint32();
		if (n < scene->mNumMeshes)
		{
			CAGE_LOG(SeverityEnum::Note, "selectMesh", stringizer() + "using mesh index " + n + ", because the input specifier is numeric");
			return n;
		}
		else
			CAGE_THROW_ERROR(Exception, "the input specifier is numeric, but the index is out of range");
	}
	std::set<uint32> candidates;
	for (uint32 meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
	{
		const aiMesh *am = scene->mMeshes[meshIndex];
		if (!am)
			continue;
		aiString aiMatName;
		scene->mMaterials[am->mMaterialIndex]->Get(AI_MATKEY_NAME, aiMatName);
		if (cage::string(am->mName.C_Str()) == inputSpec)
		{
			candidates.insert(meshIndex);
			CAGE_LOG(SeverityEnum::Note, "selectMesh", stringizer() + "considering mesh index " + meshIndex + ", because the mesh name is matching");
		}
		if (cage::string(aiMatName.C_Str()) == inputSpec)
		{
			candidates.insert(meshIndex);
			CAGE_LOG(SeverityEnum::Note, "selectMesh", stringizer() + "considering mesh index " + meshIndex + ", because the material name matches");
		}
		string comb = cage::string(am->mName.C_Str()) + "_" + cage::string(aiMatName.C_Str());
		if (comb == inputSpec)
		{
			candidates.insert(meshIndex);
			CAGE_LOG(SeverityEnum::Note, "selectMesh", stringizer() + "considering mesh index " + meshIndex + ", because the combined name matches");
		}
	}
	switch (candidates.size())
	{
	case 0:
		CAGE_THROW_ERROR(Exception, "file does not contain requested mesh");
	case 1:
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "using mesh at index " + *candidates.begin());
		return *candidates.begin();
	default:
		CAGE_THROW_ERROR(Exception, "requested name is not unique");
	}
}

Holder<AssimpSkeleton> AssimpContext::skeleton() const
{
	return detail::systemArena().createImpl<AssimpSkeleton, AssimpSkeletonImpl>(getScene());
}

Holder<AssimpContext> newAssimpContext(uint32 addFlags, uint32 removeFlags)
{
	return detail::systemArena().createImpl<AssimpContext, AssimpContextImpl>(addFlags, removeFlags);
}

void analyzeAssimp()
{
	try
	{
		Holder<AssimpContext> context = newAssimpContext(0, 0);
		const aiScene *scene = context->getScene();
		writeLine("cage-begin");
		try
		{
			// meshes
			if (scene->mNumMeshes == 1)
			{
				writeLine("scheme=mesh");
				writeLine(stringizer() + "asset=" + inputFile);
			}
			else for (uint32 i = 0; i < scene->mNumMeshes; i++)
			{
				aiMaterial *m = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
				aiString matName;
				m->Get(AI_MATKEY_NAME, matName);
				writeLine("scheme=mesh");
				writeLine(stringizer() + "asset=" + inputFile + "?" + scene->mMeshes[i]->mName.C_Str() + "_" + matName.C_Str());
			}
			// skeletons
			{
				bool found = false;
				for (uint32 i = 0; i < scene->mNumMeshes; i++)
				{
					if (scene->mMeshes[i]->HasBones())
					{
						found = true;
						break;
					}
				}
				if (found)
				{
					writeLine("scheme=skeleton");
					writeLine(stringizer() + "asset=" + inputFile + ";skeleton");
				}
			}
			// animations
			for (uint32 i = 0; i < scene->mNumAnimations; i++)
			{
				aiAnimation *a = scene->mAnimations[i];
				if (a->mNumChannels == 0)
					continue; // only support skeletal animations
				writeLine("scheme=animation");
				string n = a->mName.C_Str();
				if (n.empty())
					n = stringizer() + i;
				writeLine(stringizer() + "asset=" + inputFile + "?" + n);
			}
		}
		catch (...)
		{}
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}

vec3 conv(const aiVector3D &v)
{
	static_assert(sizeof(aiVector3D) == sizeof(cage::vec3), "assimp vector3D is not interchangeable with vec3");
	return *(vec3*)&v;
}

vec3 conv(const aiColor3D &v)
{
	static_assert(sizeof(aiColor3D) == sizeof(cage::vec3), "assimp color3D is not interchangeable with vec3");
	return *(vec3*)&v;
}

vec4 conv(const aiColor4D &v)
{
	static_assert(sizeof(aiColor4D) == sizeof(cage::vec4), "assimp color4D is not interchangeable with vec4");
	return *(vec4*)&v;
}

mat4 conv(const aiMatrix4x4 &m)
{
	static_assert(sizeof(aiMatrix4x4) == sizeof(cage::mat4), "assimp matrix4x4 is not interchangeable with mat4");
	mat4 r;
	detail::memcpy(&r, &m, sizeof(mat4));
	return transpose(r);
}

quat conv(const aiQuaternion &q)
{
	quat r;
	r.data[0] = q.x;
	r.data[1] = q.y;
	r.data[2] = q.z;
	r.data[3] = q.w;
	return r;
}

mat3 axesMatrix()
{
	string axes = properties("axes").toLower();
	if (axes.empty() || axes == "+x+y+z")
		return mat3();
	if (axes.length() != 6)
		CAGE_THROW_ERROR(Exception, "wrong axes definition: length (must be in format +x+y+z)");
	mat3 result(0, 0, 0, 0, 00, 0, 0, 0, 0);
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
			result[in * 3 + out] = real(sign);
		}
	}
	if (axesUsedCounts[0] != 1 || axesUsedCounts[1] != 1 || axesUsedCounts[2] != 1)
		CAGE_THROW_ERROR(Exception, "wrong axes definition: axes counts (must be in format +x+y+z)");
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "using axes conversion matrix: " + result);
	return result;
}

mat3 axesScaleMatrix()
{
	return axesMatrix() * properties("scale").toFloat();
}
