#include <set>
#include <map>
#include <vector>

#include "assimp.h"

#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

CAGE_ASSERT_COMPILE(sizeof(aiColor3D) == sizeof(cage::vec3), assimp_color3D_is_not_interchangeable_with_vec3);
CAGE_ASSERT_COMPILE(sizeof(aiColor4D) == sizeof(cage::vec4), assimp_color4D_is_not_interchangeable_with_vec4);
CAGE_ASSERT_COMPILE(sizeof(aiVector3D) == sizeof(cage::vec3), assimp_vector3D_is_not_interchangeable_with_vec3);

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

	typedef std::map<aiString, std::pair<uint16, uint16>, cmpAiStr> skeletonBonesCacheType; // name -> index, parent

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
		assimpContextImpl(uint32 flags) : logDebug(severityEnum::Note), logInfo(severityEnum::Info), logWarn(severityEnum::Warning), logError(severityEnum::Error)
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

			CAGE_LOG(severityEnum::Info, logComponentName, string() + "found " + scene->mNumMeshes + " meshes");
			for (unsigned i = 0; i < scene->mNumMeshes; i++)
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
				CAGE_LOG_CONTINUE(severityEnum::Note, logComponentName, string() + "index: " + i + ", object: '" + objname + "', material: '" + matname + "', contains: " + contains);
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

	public:
		skeletonBonesCacheType skeletonBonesCache;
		std::vector<uint16> skeletonBoneParents;
		std::vector<aiNode *> skeletonBoneNodes;
	};
}

extern const uint32 assimpDefaultLoadFlags =
aiProcess_JoinIdenticalVertices |
aiProcess_Triangulate |
aiProcess_LimitBoneWeights |
//aiProcess_ValidateDataStructure |
aiProcess_ImproveCacheLocality |
//aiProcess_RemoveRedundantMaterials |
aiProcess_SortByPType |
//aiProcess_FindInvalidData |
aiProcess_GenUVCoords |
aiProcess_TransformUVCoords |
//aiProcess_FindInstances |
aiProcess_FindDegenerates |
//aiProcess_OptimizeMeshes |
aiProcess_OptimizeGraph |
aiProcess_Debone |
//aiProcess_SplitLargeMeshes |
0;

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

namespace
{
	void addSkeletonBones(assimpContextImpl *impl, aiNode *n, uint16 p)
	{
		auto &cs = impl->skeletonBonesCache;
		auto &ps = impl->skeletonBoneParents;
		auto &ns = impl->skeletonBoneNodes;
		if (n->mName.length)
		{
			if (cs.count(n->mName))
				CAGE_THROW_ERROR(exception, "multiple bones with same name");
			uint32 i = numeric_cast<uint16>(ps.size());
			ps.push_back(p);
			ns.push_back(n);
			cs[n->mName] = { i , p };
			p = i;
		}
		for (uint32 i = 0; i < n->mNumChildren; i++)
			addSkeletonBones(impl, n->mChildren[i], p);
	}
}

uint16 assimpContextClass::skeletonBonesCacheSize()
{
	assimpContextImpl *impl = (assimpContextImpl*)this;
	auto &c = impl->skeletonBonesCache;
	if (!c.empty())
		return numeric_cast<uint16>(c.size());
	auto *scene = getScene();
	addSkeletonBones(impl, scene->mRootNode, -1);
	CAGE_ASSERT_RUNTIME(impl->skeletonBoneNodes.size() == impl->skeletonBoneParents.size());
	CAGE_ASSERT_RUNTIME(impl->skeletonBonesCache.size() == impl->skeletonBoneParents.size());
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "skeleton bone names cache has " + c.size() + " elements");
	return numeric_cast<uint16>(c.size());
}

const uint16 *assimpContextClass::skeletonBonesCacheParents() const
{
	const assimpContextImpl *impl = (const assimpContextImpl*)this;
	return impl->skeletonBoneParents.data();
}

aiNode *assimpContextClass::skeletonBonesCacheNode(uint32 index) const
{
	const assimpContextImpl *impl = (const assimpContextImpl*)this;
	return impl->skeletonBoneNodes[index];
}

uint16 assimpContextClass::skeletonBonesCacheIndex(const aiString &name) const
{
	const assimpContextImpl *impl = (const assimpContextImpl*)this;
	const auto &c = impl->skeletonBonesCache;
	if (c.count(name) == 0)
		CAGE_THROW_ERROR(exception, "skeleton bone name not found");
	return c.at(name).first;
}

holder<assimpContextClass> newAssimpContext(uint32 flags)
{
	return detail::systemArena().createImpl<assimpContextClass, assimpContextImpl>(flags);
}

namespace
{
	void analyzeAssimpMesh(const aiScene *scene, const aiMesh *mesh, const string &name)
	{
		writeLine("scheme=mesh");
		writeLine(string() + "asset=" + name);
		if (scene->mMeshes[0]->HasBones())
		{
			writeLine("scheme=skeleton");
			writeLine(string() + "asset=" + name + ";skeleton");
		}
	}
}

void analyzeAssimp()
{
	try
	{
		holder<assimpContextClass> context = newAssimpContext(assimpDefaultLoadFlags);
		const aiScene *scene = context->getScene();
		writeLine("cage-begin");
		try
		{
			// meshes & skeletons
			if (scene->mNumMeshes == 1)
			{
				analyzeAssimpMesh(scene, scene->mMeshes[0], inputFile);
			}
			else for (uint32 i = 0; i < scene->mNumMeshes; i++)
			{
				aiMaterial *m = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
				aiString matName;
				m->Get(AI_MATKEY_NAME, matName);
				analyzeAssimpMesh(scene, scene->mMeshes[i], inputFile + "?" + scene->mMeshes[i]->mName.C_Str() + "_" + matName.C_Str());
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
