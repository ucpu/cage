#include <set>
#include <map>
#include <vector>

#include "assimp.h"

#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

namespace
{
	struct cmpAiStr
	{
		bool operator ()(const aiString &a, const aiString &b) const
		{
			if (a.length == b.length)
				return cage::detail::memcmp(a.data, b.data, a.length) < 0;
			return a.length < b.length;
		}
	};

	class assimpSkeletonImpl : public assimpSkeletonClass
	{
	public:
		assimpSkeletonImpl(const aiScene *scene)
		{
			// find nodes and parents
			{
				std::map<aiString, aiNode*, cmpAiStr> names;
				// find names
				traverseNodes1(names, scene->mRootNode);
				std::set<aiString, cmpAiStr> interested;
				// find interested
				for (uint32 mi = 0; mi < scene->mNumMeshes; mi++)
				{
					aiMesh *m = scene->mMeshes[mi];
					for (uint32 bi = 0; bi < m->mNumBones; bi++)
					{
						aiBone *b = m->mBones[bi];
						CAGE_ASSERT_RUNTIME(names.count(b->mName) == 1);
						aiNode *n = names[b->mName];
						while (n && interested.count(n->mName) == 0)
						{
							interested.insert(n->mName);
							n = n->mParent;
						}
					}
				}
				// find nodes
				traverseNodes2(interested, scene->mRootNode, -1);
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
					CAGE_ASSERT_RUNTIME(indices.count(b->mName));
					uint16 idx = indices[b->mName];
					if (bones[idx])
					{
						CAGE_ASSERT_RUNTIME(conv(bones[idx]->mOffsetMatrix) == conv(b->mOffsetMatrix));
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
			CAGE_ASSERT_RUNTIME(num == nodes.size());
			CAGE_ASSERT_RUNTIME(num == parents.size());
			for (uint32 i = 0; i < num; i++)
			{
				if (nodes[i]->mParent)
				{
					CAGE_ASSERT_RUNTIME(parents[i] < i, parents[i], i);
					CAGE_ASSERT_RUNTIME(parent(nodes[i]) == nodes[parents[i]]);
				}
				else
				{
					CAGE_ASSERT_RUNTIME(parents[i] == (uint16)-1);
				}
				if (bones[i])
				{
					CAGE_ASSERT_RUNTIME(bones[i]->mName == nodes[i]->mName);
				}
				if (nodes[i]->mName.length)
				{
					CAGE_ASSERT_RUNTIME(indices.count(nodes[i]->mName) == 1);
					CAGE_ASSERT_RUNTIME(indices[nodes[i]->mName] == i);
				}
			}
		}

		void traverseNodes1(std::map<aiString, aiNode*, cmpAiStr> &names, aiNode *n)
		{
			if (n->mName.length)
				names[n->mName] = n;
			for (uint32 i = 0; i < n->mNumChildren; i++)
				traverseNodes1(names, n->mChildren[i]);
		}

		void traverseNodes2(const std::set<aiString, cmpAiStr> &interested, aiNode *n, uint16 pi)
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
		std::map<aiString, uint16, cmpAiStr> indices;
	};

	class cageIoStream : public Assimp::IOStream
	{
	public:
		cageIoStream(cage::holder<cage::fileClass> r) : r(templates::move(r))
		{}

		virtual ~cageIoStream()
		{}

		virtual size_t Read(void* pvBuffer, size_t pSize, size_t pCount)
		{
			uint64 avail = r->size() - r->tell();
			uint64 size = pSize * pCount;
			while (size > avail)
			{
				size -= pSize;
				pCount--;
			}
			r->read(pvBuffer, size);
			return pCount;
		}

		virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount)
		{
			CAGE_THROW_ERROR(exception, "cageIOStrea::Write is not meant for use");
		}

		virtual aiReturn Seek(size_t pOffset, aiOrigin pOrigin)
		{
			switch (pOrigin)
			{
			case aiOrigin_SET: r->seek(pOffset); break;
			case aiOrigin_CUR: r->seek(r->tell() + pOffset); break;
			case aiOrigin_END: r->seek(r->size() + pOffset); break;
			default: CAGE_THROW_CRITICAL(exception, "cageIOStream::Seek: unknown pOrigin");
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
			CAGE_THROW_ERROR(exception, "cageIOStream::Flush is not meant for use");
		}

	private:
		cage::holder<cage::fileClass> r;
	};

	class cageIoSystem : public Assimp::IOSystem
	{
	public:
		cageIoSystem()
		{
			f = newFilesystem();
			f->changeDir(pathJoin(inputDirectory, pathExtractPath(inputFile)));
		}

		virtual bool Exists(const char *pFile) const
		{
			return f->exists(pFile);
		}

		virtual char getOsSeparator() const
		{
			return '/';
		}

		virtual Assimp::IOStream *Open(const char *pFile, const char *pMode = "rb")
		{
			if (::cage::string(pMode) != "rb")
				CAGE_THROW_ERROR(exception, "cageIoSystem::Open: only support rb mode");
			writeLine(cage::string("use = ") + pathJoin(pathExtractPath(inputFile), pFile));
			return new cageIoStream(f->openFile(pFile, fileMode(true, false)));
		}

		virtual void Close(Assimp::IOStream *pFile)
		{
			delete (cageIoStream*)pFile;
		}

		virtual bool ComparePaths(const char* one, const char* second) const
		{
			CAGE_THROW_ERROR(exception, "cageIOsystem::ComparePaths is not meant for use");
		}

	private:

		cage::holder<filesystemClass> f;
	};

	class cageLogStream : public Assimp::LogStream
	{
	public:
		cage::severityEnum severity;
		cageLogStream(cage::severityEnum severity) : severity(severity)
		{}
		virtual void write(const char* message)
		{
			string m = message;
			if (m.pattern("", "", "\n"))
				m = m.subString(0, m.length() - 1);
			CAGE_LOG(severity, "assimp", m);
		}
	};

	struct assimpContextImpl : public assimpContextClass
	{
		static const uint32 assimpDefaultLoadFlags =
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
			aiProcess_Debone |
			//aiProcess_SplitLargeMeshes |
			0;

		static const uint32 assimpBakeLoadFlags =
			aiProcess_RemoveRedundantMaterials |
			//aiProcess_FindInstances |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			0;

		assimpContextImpl(uint32 addFlags, uint32 removeFlags) : logDebug(severityEnum::Note), logInfo(severityEnum::Info), logWarn(severityEnum::Warning), logError(severityEnum::Error)
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

			try
			{
				uint32 flags = assimpDefaultLoadFlags;
				if (properties("bake_model").toBool())
					flags |= assimpBakeLoadFlags;
				flags |= addFlags;
				flags &= ~removeFlags;
				CAGE_LOG(severityEnum::Info, logComponentName, cage::string() + "assimp loading flags: " + flags);

				imp.SetIOHandler(&this->ioSystem);
				if (!imp.ReadFile(pathExtractFilename(inputFile).c_str(), flags))
				{
					CAGE_LOG(severityEnum::Note, "exception", cage::string(imp.GetErrorString()));
					CAGE_THROW_ERROR(exception, "assimp loading failed");
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
				CAGE_THROW_ERROR(exception, "scene is null");

			if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == AI_SCENE_FLAGS_INCOMPLETE)
				CAGE_THROW_ERROR(exception, "the scene is incomplete");

			// print meshes
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "found " + scene->mNumMeshes + " meshes");
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
					contains += "uv ";
				if (am->HasTangentsAndBitangents())
					contains += "tangents ";
				if (am->HasVertexColors(0))
					contains += "colors ";
				CAGE_LOG_CONTINUE(severityEnum::Note, logComponentName, string() + "index: " + i + ", object: '" + objname + "', material: '" + matname + "', contains: " + contains);
			}

			// print animations
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "found " + scene->mNumAnimations + " animations");
			for (uint32 i = 0; i < scene->mNumAnimations; i++)
			{
				const aiAnimation *ani = scene->mAnimations[i];
				CAGE_LOG_CONTINUE(severityEnum::Info, logComponentName, string() + "index: " + i + ", animation: '" + ani->mName.data + "', channels: " + ani->mNumChannels);
			}
		};

		~assimpContextImpl()
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
		cageIoSystem ioSystem;
		cageLogStream logDebug, logInfo, logWarn, logError;
		Assimp::Importer imp;
	};
}

uint16 assimpSkeletonClass::bonesCount() const
{
	const assimpSkeletonImpl *impl = (const assimpSkeletonImpl*)this;
	CAGE_ASSERT_RUNTIME(impl->bones.size() == impl->nodes.size());
	CAGE_ASSERT_RUNTIME(impl->parents.size() == impl->nodes.size());
	return numeric_cast<uint16>(impl->bones.size());
}

aiNode *assimpSkeletonClass::node(aiBone *bone) const
{
	return node(index(bone));
}

aiNode *assimpSkeletonClass::node(uint16 index) const
{
	const assimpSkeletonImpl *impl = (const assimpSkeletonImpl*)this;
	if (index == (uint16)-1)
		return nullptr;
	return impl->nodes[index];
}

aiBone *assimpSkeletonClass::bone(aiNode *node) const
{
	return bone(index(node));
}

aiBone *assimpSkeletonClass::bone(uint16 index) const
{
	const assimpSkeletonImpl *impl = (const assimpSkeletonImpl*)this;
	if (index == (uint16)-1)
		return nullptr;
	return impl->bones[index];
}

uint16 assimpSkeletonClass::index(const aiString &name) const
{
	const assimpSkeletonImpl *impl = (const assimpSkeletonImpl*)this;
	if (!name.length)
		return (uint16)-1;
	if (!impl->indices.count(name))
		return (uint16)-1;
	return impl->indices.at(name);
}

uint16 assimpSkeletonClass::index(aiNode *node) const
{
	if (!node)
		return (uint16)-1;
	return index(node->mName);
}

uint16 assimpSkeletonClass::index(aiBone *bone) const
{
	if (!bone)
		return (uint16)-1;
	return index(bone->mName);
}

uint16 assimpSkeletonClass::parent(uint16 index) const
{
	const assimpSkeletonImpl *impl = (const assimpSkeletonImpl*)this;
	if (index == (uint16)-1)
		return (uint16)-1;
	return impl->parents[index];
}

aiBone *assimpSkeletonClass::parent(aiBone *bone) const
{
	return this->bone(parent(index(bone)));
}

aiNode *assimpSkeletonClass::parent(aiNode *node) const
{
	return this->node(parent(index(node)));
}

const aiScene *assimpContextClass::getScene() const
{
	return ((assimpContextImpl*)this)->getScene();
}

uint32 assimpContextClass::selectMesh() const
{
	const aiScene *scene = getScene();
	if (scene->mNumMeshes == 1 && inputSpec.empty())
	{
		CAGE_LOG(severityEnum::Note, "selectMesh", "using the first mesh, because it is the only mesh available");
		return 0;
	}
	std::set<uint32> candidates;
	if (inputSpec.isInteger(false))
	{
		uint32 n = inputSpec.toUint32();
		if (n < scene->mNumMeshes)
		{
			candidates.insert(n);
			CAGE_LOG(severityEnum::Note, "selectMesh", string() + "considering mesh index " + n + ", because the input specifier is numeric");
		}
		else
			CAGE_LOG(severityEnum::Note, "selectMesh", string() + "the input specifier is numeric, but the index is out of range, so not considered as an index");
	}
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
			CAGE_LOG(severityEnum::Note, "selectMesh", string() + "considering mesh index " + meshIndex + ", because the mesh name is matching");
		}
		if (cage::string(aiMatName.C_Str()) == inputSpec)
		{
			candidates.insert(meshIndex);
			CAGE_LOG(severityEnum::Note, "selectMesh", string() + "considering mesh index " + meshIndex + ", because the material name matches");
		}
		string comb = cage::string(am->mName.C_Str()) + "_" + cage::string(aiMatName.C_Str());
		if (comb == inputSpec)
		{
			candidates.insert(meshIndex);
			CAGE_LOG(severityEnum::Note, "selectMesh", string() + "considering mesh index " + meshIndex + ", because the combined name matches");
		}
	}
	switch (candidates.size())
	{
	case 0:
		CAGE_THROW_ERROR(exception, "file does not contain requested mesh");
	case 1:
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "using mesh at index " + *candidates.begin());
		return *candidates.begin();
	default:
		CAGE_THROW_ERROR(exception, "requested name is not unique");
	}
}

holder<assimpSkeletonClass> assimpContextClass::skeleton() const
{
	return detail::systemArena().createImpl<assimpSkeletonClass, assimpSkeletonImpl>(getScene());
}

holder<assimpContextClass> newAssimpContext(uint32 addFlags, uint32 removeFlags)
{
	return detail::systemArena().createImpl<assimpContextClass, assimpContextImpl>(addFlags, removeFlags);
}

void analyzeAssimp()
{
	try
	{
		holder<assimpContextClass> context = newAssimpContext(0, 0);
		const aiScene *scene = context->getScene();
		writeLine("cage-begin");
		try
		{
			// meshes
			if (scene->mNumMeshes == 1)
			{
				writeLine("scheme=mesh");
				writeLine(string() + "asset=" + inputFile);
			}
			else for (uint32 i = 0; i < scene->mNumMeshes; i++)
			{
				aiMaterial *m = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
				aiString matName;
				m->Get(AI_MATKEY_NAME, matName);
				writeLine("scheme=mesh");
				writeLine(string() + "asset=" + inputFile + "?" + scene->mMeshes[i]->mName.C_Str() + "_" + matName.C_Str());
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
					writeLine(string() + "asset=" + inputFile + ";skeleton");
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
					n = i;
				writeLine(string() + "asset=" + inputFile + "?" + n);
			}
		}
		catch (...)
		{
			writeLine("cage-end");
		}
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}

vec3 conv(const aiVector3D &v)
{
	CAGE_ASSERT_COMPILE(sizeof(aiVector3D) == sizeof(cage::vec3), assimp_vector3D_is_not_interchangeable_with_vec3);
	return *(vec3*)&v;
}

vec3 conv(const aiColor3D &v)
{
	CAGE_ASSERT_COMPILE(sizeof(aiColor3D) == sizeof(cage::vec3), assimp_color3D_is_not_interchangeable_with_vec3);
	return *(vec3*)&v;
}

vec4 conv(const aiColor4D &v)
{
	CAGE_ASSERT_COMPILE(sizeof(aiColor4D) == sizeof(cage::vec4), assimp_color4D_is_not_interchangeable_with_vec4);
	return *(vec4*)&v;
}

mat4 conv(const aiMatrix4x4 &m)
{
	CAGE_ASSERT_COMPILE(sizeof(aiMatrix4x4) == sizeof(cage::mat4), assimp_matrix4x4_is_not_interchangeable_with_mat4);
	mat4 r;
	detail::memcpy(&r, &m, sizeof(mat4));
	//r[15] = 1;
	return r.transpose();
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
		CAGE_THROW_ERROR(exception, "wrong axes definition: length (must be in format +x+y+z)");
	mat3 result(0, 0, 0, 0, 00, 0, 0, 0, 0);
	int sign = 0;
	uint32 axesUsedCounts[3] = { 0, 0, 0 };
	for (uint32 i = 0; i < 6; i++)
	{
		char c = axes[i];
		if (i % 2 == 0)
		{ // signs
			if (c != '+' && c != '-')
				CAGE_THROW_ERROR(exception, "wrong axes definition: signs (must be in format +x+y+z)");
			if (c == '+')
				sign = 1;
			else
				sign = -1;
		}
		else
		{ // axes
			uint32 out = i / 2;
			uint32 in = -1;
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
				CAGE_THROW_ERROR(exception, "wrong axes definition: invalid axis (must be in format +x+y+z)");
			}
			result[in * 3 + out] = real(sign);
		}
	}
	if (axesUsedCounts[0] != 1 || axesUsedCounts[1] != 1 || axesUsedCounts[2] != 1)
		CAGE_THROW_ERROR(exception, "wrong axes definition: axes counts (must be in format +x+y+z)");
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "using axes conversion matrix: " + result);
	return result;
}

mat3 axesScaleMatrix()
{
	return axesMatrix() * properties("scale").toFloat();
}
