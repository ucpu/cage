#include <set>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/GltfMaterial.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cage-core/color.h>
#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/string.h>

namespace cage
{
	StringPointer meshImportTextureTypeToString(MeshImportTextureType type)
	{
		switch (type)
		{
			case MeshImportTextureType::None:
				return "none";
			case MeshImportTextureType::Albedo:
				return "albedo";
			case MeshImportTextureType::Normal:
				return "normal";
			case MeshImportTextureType::Special:
				return "special";
			case MeshImportTextureType::Custom:
				return "custom";
			case MeshImportTextureType::AmbientOcclusion:
				return "ambient occlusion";
			case MeshImportTextureType::Anisotropy:
				return "anisotropy";
			case MeshImportTextureType::Bump:
				return "bump";
			case MeshImportTextureType::Clearcoat:
				return "clearcoat";
			case MeshImportTextureType::Emission:
				return "emission";
			case MeshImportTextureType::GltfPbr:
				return "gltf pbr";
			case MeshImportTextureType::Mask:
				return "mask";
			case MeshImportTextureType::Metallic:
				return "metallic";
			case MeshImportTextureType::Opacity:
				return "opacity";
			case MeshImportTextureType::Roughness:
				return "roughness";
			case MeshImportTextureType::Sheen:
				return "sheen";
			case MeshImportTextureType::Shininess:
				return "shininess";
			case MeshImportTextureType::Specular:
				return "specular";
			case MeshImportTextureType::Transmission:
				return "transmission";
		}
		return "unknown";
	}

	namespace privat
	{
		void meshImportConvertToCageFormats(MeshImportPart &part);
	}

	namespace
	{
		class CageLogStream : public Assimp::LogStream
		{
		public:
			const cage::SeverityEnum severity;

			CageLogStream(cage::SeverityEnum severity) : severity(severity) {}

			void write(const char *message) override
			{
				String m = message;
				if (isPattern(m, "", "", "\n"))
					m = subString(m, 0, m.length() - 1);
				CAGE_LOG(severity, "assimp", m);
			}
		};

		int initializeAssimpLogger()
		{
#ifdef CAGE_DEBUG
			static constexpr Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
#else
			static constexpr Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
#endif
			Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_FILE);
			Assimp::DefaultLogger::get()->attachStream(new CageLogStream(SeverityEnum::Note), Assimp::Logger::Debugging);
			Assimp::DefaultLogger::get()->attachStream(new CageLogStream(SeverityEnum::Info), Assimp::Logger::Info);
			Assimp::DefaultLogger::get()->attachStream(new CageLogStream(SeverityEnum::Warning), Assimp::Logger::Warn);
			Assimp::DefaultLogger::get()->attachStream(new CageLogStream(SeverityEnum::Error), Assimp::Logger::Err);

			return 0;
		}

		Vec3 conv(const aiVector3D &v)
		{
			static_assert(sizeof(aiVector3D) == sizeof(cage::Vec3));
			return *(Vec3 *)&v;
		}

		Vec3 conv(const aiColor3D &v)
		{
			static_assert(sizeof(aiColor3D) == sizeof(cage::Vec3));
			return *(Vec3 *)&v;
		}

		Mat4 conv(const aiMatrix4x4 &m)
		{
			static_assert(sizeof(aiMatrix4x4) == sizeof(cage::Mat4));
			Mat4 r;
			detail::memcpy(&r, &m, sizeof(Mat4));
			return transpose(r);
		}

		Quat conv(const aiQuaternion &q)
		{
			Quat r;
			r.data[0] = q.x;
			r.data[1] = q.y;
			r.data[2] = q.z;
			r.data[3] = q.w;
			return r;
		}

		String convStrTruncate(const aiString &str, uint32 maxLen = cage::String::MaxLength / 2)
		{
			String s;
			s.rawLength() = min(str.length, maxLen);
			if (s.length())
				detail::memcpy(s.rawData(), str.data, s.length());
			s.rawData()[s.length()] = 0;
			return s;
		}

		Aabb enlarge(const Aabb &box)
		{
			const Vec3 c = box.center();
			const Vec3 a = box.a - c;
			const Vec3 b = box.b - c;
			static constexpr Real s = 1.1;
			return Aabb(a * s + c, b * s + c);
		}

		uint32 convertPrimitiveType(int primitiveTypes)
		{
			switch (primitiveTypes)
			{
				case aiPrimitiveType_POINT:
					return 1;
				case aiPrimitiveType_LINE:
					return 2;
				case aiPrimitiveType_TRIANGLE:
					return 3;
				default:
					CAGE_THROW_ERROR(Exception, "mesh has invalid primitive type");
			}
		}

		class CageIoStream : public Assimp::IOStream
		{
		public:
			explicit CageIoStream(cage::Holder<cage::File> r) : r(std::move(r)) {}

			virtual ~CageIoStream() {}

			size_t Read(void *pvBuffer, size_t pSize, size_t pCount) override
			{
				uint64 avail = r->size() - r->tell();
				uint64 size = pSize * pCount;
				while (size > avail)
				{
					size -= pSize;
					pCount--;
				}
				r->read({ (char *)pvBuffer, (char *)pvBuffer + size });
				return pCount;
			}

			size_t Write(const void *pvBuffer, size_t pSize, size_t pCount) override { CAGE_THROW_CRITICAL(Exception, "cageIOStream::Write"); }

			aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override
			{
				switch (pOrigin)
				{
					case aiOrigin_SET:
						r->seek(pOffset);
						break;
					case aiOrigin_CUR:
						r->seek(r->tell() + pOffset);
						break;
					case aiOrigin_END:
						r->seek(r->size() + pOffset);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "cageIOStream::Seek: unknown pOrigin");
				}
				return aiReturn_SUCCESS;
			}

			size_t Tell() const override { return (size_t)r->tell(); }

			size_t FileSize() const override { return (size_t)r->size(); }

			void Flush() override { CAGE_THROW_CRITICAL(Exception, "cageIOStream::Flush"); }

		private:
			cage::Holder<cage::File> r;
		};

		class CageIoSystem : public Assimp::IOSystem
		{
		public:
			bool Exists(const char *pFile) const override { return any(pathType(pathToAbs(pFile)) & PathTypeFlags::File); }

			char getOsSeparator() const override { return '/'; }

			Assimp::IOStream *Open(const char *pFile, const char *pMode = "rb") override
			{
				if (::cage::String(pMode) != "rb")
					CAGE_THROW_ERROR(Exception, "CageIoSystem::Open: only support rb mode");
				if (!pathIsAbs(pFile) || pathSimplify(pFile) != String(pFile))
				{
					CAGE_LOG_THROW(Stringizer() + "path: " + pFile);
					CAGE_THROW_ERROR(Exception, "assimp is accessing invalid path");
				}
				paths.insert(pFile);
				return new CageIoStream(readFile(pFile));
			}

			void Close(Assimp::IOStream *pFile) override { delete (CageIoStream *)pFile; }

			bool ComparePaths(const char *one, const char *second) const override { CAGE_THROW_CRITICAL(Exception, "cageIOsystem::ComparePaths"); }

			std::set<String> paths;
		};

		struct CmpAiStr
		{
			bool operator()(const aiString &a, const aiString &b) const
			{
				if (a.length == b.length)
					return cage::detail::memcmp(a.data, b.data, a.length) < 0;
				return a.length < b.length;
			}
		};

		class AssimpSkeleton : private Immovable
		{
		public:
			AssimpSkeleton(const aiScene *scene)
			{
				// find nodes and parents
				{
					std::map<aiString, aiNode *, CmpAiStr> names;
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

			void traverseNodes1(std::map<aiString, aiNode *, CmpAiStr> &names, aiNode *n)
			{
				if (n->mName.length)
				{
					if (names.count(n->mName))
					{ // make the name unique
						n->mName = String(Stringizer() + n->mName.C_Str() + "_" + n).c_str();
						CAGE_LOG_DEBUG(SeverityEnum::Warning, "assimp", Stringizer() + "renamed a node: " + n->mName.C_Str());
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

			uint16 bonesCount() const
			{
				CAGE_ASSERT(bones.size() == nodes.size());
				CAGE_ASSERT(parents.size() == nodes.size());
				return numeric_cast<uint16>(bones.size());
			}

			aiNode *node(aiBone *bone) const { return node(index(bone)); }

			aiNode *node(uint16 index) const
			{
				if (index == m)
					return nullptr;
				return nodes[index];
			}

			aiBone *bone(aiNode *node) const { return bone(index(node)); }

			aiBone *bone(uint16 index) const
			{
				if (index == m)
					return nullptr;
				return bones[index];
			}

			uint16 index(const aiString &name) const
			{
				if (!name.length)
					return m;
				if (!indices.count(name))
					return m;
				return indices.at(name);
			}

			uint16 index(aiNode *node) const
			{
				if (!node)
					return m;
				return index(node->mName);
			}

			uint16 index(aiBone *bone) const
			{
				if (!bone)
					return m;
				return index(bone->mName);
			}

			uint16 parent(uint16 index) const
			{
				if (index == m)
					return m;
				return parents[index];
			}

			aiBone *parent(aiBone *bone_) const { return bone(parent(index(bone_))); }

			// this may skip some nodes in the original hierarchy - this function returns node that corresponds to another bone
			aiNode *parent(aiNode *node_) const { return node(parent(index(node_))); }

		private:
			std::vector<aiBone *> bones;
			std::vector<aiNode *> nodes;
			std::vector<uint16> parents;
			std::map<aiString, uint16, CmpAiStr> indices;
		};

		struct AssimpContext : private Immovable
		{
			explicit AssimpContext(const String &filename, const MeshImportConfig &config) : inputFile(filename), config(config)
			{
				static int assimpLogger = initializeAssimpLogger();
				(void)assimpLogger;

				//imp.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, true);
				imp.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 30);
				if (config.trianglesOnly)
					imp.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE | aiPrimitiveType_POLYGON);

				// initial import
				try
				{
					imp.SetIOHandler(&this->ioSystem);
					if (!imp.ReadFile(inputFile.c_str(), 0))
					{
						CAGE_LOG_THROW(cage::String(imp.GetErrorString()));
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

				// discard skeleton and animations
				if (config.discardSkeleton)
				{
					aiScene *sc = const_cast<aiScene *>(scene);

					// remove animations
					sc->mNumAnimations = 0;
					sc->mAnimations = nullptr;

					// remove bones
					for (uint32 i = 0; i < scene->mNumMeshes; i++)
					{
						aiMesh *mesh = scene->mMeshes[i];
						mesh->mNumBones = 0;
						mesh->mBones = nullptr;
					}
				}

				// apply post-processing
				{
					static constexpr uint32 DefaultFlags = aiProcess_FindDegenerates | aiProcess_GenUVCoords | aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights | aiProcess_OptimizeGraph | aiProcess_SortByPType | aiProcess_TransformUVCoords | aiProcess_Triangulate;
					uint32 flags = DefaultFlags;
					if (config.mergeParts)
					{
						//	flags |= aiProcess_FindInstances | aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials;
						flags |= aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials;
					}
					if (config.generateNormals)
						flags |= aiProcess_GenSmoothNormals;
					scene = imp.ApplyPostProcessing(flags);
				}

				// validate
				if (!scene)
					CAGE_THROW_ERROR(Exception, "scene is null");
				if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == AI_SCENE_FLAGS_INCOMPLETE)
					CAGE_THROW_ERROR(Exception, "the scene is incomplete");
				if (config.discardSkeleton)
				{
					CAGE_ASSERT(scene->mNumAnimations == 0);
					CAGE_ASSERT(!scene->mAnimations);
					for (uint32 i = 0; i < scene->mNumMeshes; i++)
					{
						const aiMesh *mesh = scene->mMeshes[i];
						CAGE_ASSERT(mesh->mNumBones == 0);
						CAGE_ASSERT(!mesh->mBones);
					}
				}

				if (config.verbose)
				{
					// print models
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "found " + scene->mNumMeshes + " models");
					for (uint32 i = 0; i < scene->mNumMeshes; i++)
					{
						const aiMesh *am = scene->mMeshes[i];
						const String objname = convStrTruncate(am->mName);
						aiString aiMatName;
						scene->mMaterials[am->mMaterialIndex]->Get(AI_MATKEY_NAME, aiMatName);
						const String matname = convStrTruncate(aiMatName);
						String contains;
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
						if (am->HasVertexColors(0))
							contains += "colors ";
						CAGE_LOG_CONTINUE(SeverityEnum::Note, "meshImport", Stringizer() + "index: " + i + ", object: " + objname + ", material: " + matname + ", contains: " + contains);
					}

					// print animations
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "found " + scene->mNumAnimations + " animations");
					for (uint32 i = 0; i < scene->mNumAnimations; i++)
					{
						const aiAnimation *ani = scene->mAnimations[i];
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "meshImport", Stringizer() + "index: " + i + ", animation: " + convStrTruncate(ani->mName) + ", channels: " + ani->mNumChannels);
					}
				}

				{
					bool hasBones = false;
					for (uint32 i = 0; i < scene->mNumMeshes; i++)
					{
						if (scene->mMeshes[i]->HasBones())
						{
							hasBones = true;
							break;
						}
					}
					if (hasBones)
						skeleton = systemMemory().createHolder<AssimpSkeleton>(scene);
				}
			};

			Holder<SkeletonRig> skeletonRig() const
			{
				// print the nodes hierarchy
				//CAGE_LOG(SeverityEnum::Info, "meshImport", "full node hierarchy:");
				//printHierarchy(+skeleton, imp.GetScene()->mRootNode, 0);

				const Mat4 globalInverse = inverse(conv(imp.GetScene()->mRootNode->mTransformation));
				const uint32 bonesCount = skeleton->bonesCount();

				std::vector<uint16> ps;
				std::vector<Mat4> bs;
				std::vector<Mat4> is;
				ps.reserve(bonesCount);
				bs.reserve(bonesCount);
				is.reserve(bonesCount);

				// find parents and matrices
				for (uint32 i = 0; i < bonesCount; i++)
				{
					const aiNode *n = skeleton->node(i);
					const aiBone *b = skeleton->bone(i);
					const Mat4 t = conv(n->mTransformation);
					const Mat4 o = (b ? conv(b->mOffsetMatrix) : Mat4());
					CAGE_ASSERT(t.valid() && o.valid());
					ps.push_back(skeleton->parent(i));
					bs.push_back(t);
					is.push_back(o);
				}

				Holder<SkeletonRig> rig = newSkeletonRig();
				rig->skeletonData(globalInverse, ps, bs, is);
				return rig;
			}

			Holder<SkeletalAnimation> animation(const uint32 chosenAnimationIndex) const
			{
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "converting assimp animation at index: " + chosenAnimationIndex);

				struct Bone
				{
					std::vector<Real> posTimes;
					std::vector<Vec3> posVals;
					std::vector<Real> rotTimes;
					std::vector<Quat> rotVals;
					std::vector<Real> sclTimes;
					std::vector<Vec3> sclVals;
				};

				const aiAnimation *ani = imp.GetScene()->mAnimations[chosenAnimationIndex];
				if (ani->mNumChannels == 0 || ani->mNumMeshChannels != 0 || ani->mNumMorphMeshChannels != 0)
					CAGE_THROW_ERROR(Exception, "the animation has unsupported type");

				if (config.verbose)
				{
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "duration: " + ani->mDuration + " ticks");
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "ticks per second: " + ani->mTicksPerSecond);
				}

				const uint64 duration = numeric_cast<uint64>(1e6 * ani->mDuration / (ani->mTicksPerSecond > 0 ? ani->mTicksPerSecond : 25.0));
				const uint32 skeletonBonesCount = skeleton->bonesCount();

				uint32 totalKeys = 0;
				std::vector<Bone> bones;
				bones.reserve(ani->mNumChannels);
				std::vector<uint16> boneIndices;
				boneIndices.reserve(ani->mNumChannels);
				for (uint32 channelIndex = 0; channelIndex < ani->mNumChannels; channelIndex++)
				{
					aiNodeAnim *n = ani->mChannels[channelIndex];
					uint16 idx = skeleton->index(n->mNodeName);
					if (idx == m)
					{
						CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "channel index: " + channelIndex + ", name: " + n->mNodeName.data + ", has no corresponding bone and will be ignored");
						continue;
					}
					boneIndices.push_back(idx);
					bones.emplace_back();
					Bone &b = *bones.rbegin();
					// positions
					b.posTimes.reserve(n->mNumPositionKeys);
					b.posVals.reserve(n->mNumPositionKeys);
					for (uint32 i = 0; i < n->mNumPositionKeys; i++)
					{
						aiVectorKey &k = n->mPositionKeys[i];
						b.posTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
						b.posVals.push_back(conv(k.mValue));
					}
					totalKeys += n->mNumPositionKeys;
					// rotations
					b.rotTimes.reserve(n->mNumRotationKeys);
					b.rotVals.reserve(n->mNumRotationKeys);
					for (uint32 i = 0; i < n->mNumRotationKeys; i++)
					{
						aiQuatKey &k = n->mRotationKeys[i];
						b.rotTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
						b.rotVals.push_back(conv(k.mValue));
					}
					totalKeys += n->mNumRotationKeys;
					// scales
					b.sclTimes.reserve(n->mNumScalingKeys);
					b.sclVals.reserve(n->mNumScalingKeys);
					for (uint32 i = 0; i < n->mNumScalingKeys; i++)
					{
						aiVectorKey &k = n->mScalingKeys[i];
						b.sclTimes.push_back(numeric_cast<float>(k.mTime / ani->mDuration));
						b.sclVals.push_back(conv(k.mValue));
					}
					totalKeys += n->mNumScalingKeys;
				}
				const uint16 animationBonesCount = numeric_cast<uint16>(bones.size());
				if (config.verbose)
				{
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "animated bones: " + animationBonesCount);
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "total keys: " + totalKeys);
				}

				Holder<SkeletalAnimation> anim = newSkeletalAnimation();
				anim->duration(duration);
				{
					std::vector<uint16> mapping;
					mapping.resize(skeletonBonesCount, m);
					for (uint16 i = 0; i < animationBonesCount; i++)
					{
						CAGE_ASSERT(mapping[boneIndices[i]] == m);
						mapping[boneIndices[i]] = i;
					}
					anim->channelsMapping(skeletonBonesCount, animationBonesCount, mapping);
				}
				{
					std::vector<PointerRange<const Real>> times;
					times.reserve(animationBonesCount);
					{
						std::vector<PointerRange<const Vec3>> positions;
						positions.reserve(animationBonesCount);
						for (const Bone &b : bones)
						{
							times.push_back(b.posTimes);
							positions.push_back(b.posVals);
						}
						anim->positionsData(times, positions);
						times.clear();
					}
					{
						std::vector<PointerRange<const Quat>> rotations;
						rotations.reserve(animationBonesCount);
						for (const Bone &b : bones)
						{
							times.push_back(b.rotTimes);
							rotations.push_back(b.rotVals);
						}
						anim->rotationsData(times, rotations);
						times.clear();
					}
					{
						std::vector<PointerRange<const Vec3>> scales;
						scales.reserve(animationBonesCount);
						for (const Bone &b : bones)
						{
							times.push_back(b.sclTimes);
							scales.push_back(b.sclVals);
						}
						anim->scaleData(times, scales);
						times.clear();
					}
				}
				return anim;
			}

			Holder<Mesh> mesh(const uint32 chosenMeshIndex) const
			{
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "converting assimp mesh at index: " + chosenMeshIndex);

				const aiMesh *am = imp.GetScene()->mMeshes[chosenMeshIndex];

				if (am->GetNumUVChannels() > 1)
					CAGE_LOG(SeverityEnum::Warning, "meshImport", "multiple uv channels are not supported - using only the first");

				const uint32 indicesPerPrimitive = convertPrimitiveType(am->mPrimitiveTypes);
				const uint32 verticesCount = am->mNumVertices;
				const uint32 indicesCount = am->mNumFaces * indicesPerPrimitive;

				if (config.verbose)
				{
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "indices per primitive: " + indicesPerPrimitive);
					CAGE_LOG(SeverityEnum::Info, "meshImport", cage::Stringizer() + "vertices count: " + verticesCount);
					CAGE_LOG(SeverityEnum::Info, "meshImport", cage::Stringizer() + "indices count: " + indicesCount);
				}

				Holder<Mesh> poly = newMesh();
				switch (indicesPerPrimitive)
				{
					case 1:
						poly->type(MeshTypeEnum::Points);
						break;
					case 2:
						poly->type(MeshTypeEnum::Lines);
						break;
					case 3:
						poly->type(MeshTypeEnum::Triangles);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid mesh type enum");
				}

				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying vertex positions");
					std::vector<Vec3> ps;
					ps.reserve(verticesCount);
					for (uint32 i = 0; i < verticesCount; i++)
					{
						Vec3 p = conv(am->mVertices[i]);
						ps.push_back(p);
					}
					poly->positions(ps);
				}

				if (am->HasNormals())
				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying normals");
					std::vector<Vec3> ps;
					ps.reserve(verticesCount);
					for (uint32 i = 0; i < verticesCount; i++)
					{
						Vec3 n = conv(am->mNormals[i]);
						static constexpr const char Name[] = "normal";
						ps.push_back(fixUnitVector(Name, n));
					}
					poly->normals(ps);
				}

				if (am->HasBones())
				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying bones");

					CAGE_ASSERT(am->mNumBones > 0);
					std::vector<Vec4i> boneIndices;
					boneIndices.resize(verticesCount, Vec4i((sint32)m));
					std::vector<Vec4> boneWeights;
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
						Real sum = 0;
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
							const Real f = 1 / sum;
							CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "renormalizing bone weights for " + i + "th vertex by factor " + f);
							for (uint32 j = 0; j < 4; j++)
								boneWeights[i][j] *= f;
						}
					}

					CAGE_ASSERT(maxBoneId <= skeleton->bonesCount());
					poly->boneIndices(boneIndices);
					poly->boneWeights(boneWeights);
				}

				if (am->HasTextureCoords(0))
				{
					if (am->mNumUVComponents[0] == 3)
					{
						if (config.verbose)
							CAGE_LOG(SeverityEnum::Info, "meshImport", "copying 3D uvs");
						std::vector<Vec3> ts;
						ts.reserve(verticesCount);
						for (uint32 i = 0; i < verticesCount; i++)
							ts.push_back(conv(am->mTextureCoords[0][i]));
						poly->uvs3(ts);
					}
					else
					{
						if (config.verbose)
							CAGE_LOG(SeverityEnum::Info, "meshImport", "copying 2D uvs");
						CAGE_ASSERT(am->mNumUVComponents[0] == 2);
						std::vector<Vec2> ts;
						ts.reserve(verticesCount);
						for (uint32 i = 0; i < verticesCount; i++)
							ts.push_back(Vec2(conv(am->mTextureCoords[0][i])));
						poly->uvs(ts);
					}
				}

				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying indices");
					std::vector<uint32> inds;
					inds.reserve(indicesCount);
					for (uint32 i = 0; i < am->mNumFaces; i++)
					{
						for (uint32 j = 0; j < indicesPerPrimitive; j++)
							inds.push_back(numeric_cast<uint32>(am->mFaces[i].mIndices[j]));
					}
					poly->indices(inds);
				}

				return poly;
			}

			using Textures = PointerRangeHolder<MeshImportTexture>;

			static String separateDetail(String &p)
			{
				const uint32 sep = min(find(p, '?'), find(p, ';'));
				const String detail = subString(p, sep, m);
				p = subString(p, 0, sep);
				return detail;
			}

			String convertPath(const String &pathBase, String n) const
			{
				String detail = separateDetail(n);
				if (n.empty())
					return n + detail;
				if (n[0] == '/')
					n = pathJoin(config.rootPath, trim(n, true, false, "/"));
				else
					n = pathJoin(pathBase, n);
				n = pathToAbs(n);
				return n + detail;
			}

			template<MeshImportTextureType Type>
			void loadTextureCage(const String &pathBase, Ini *ini, Textures &textures, PointerRange<MeshImportTexture> replacements)
			{
				String n = ini->getString("textures", String(meshImportTextureTypeToString(Type)));
				n = convertPath(pathBase, n);
				if (n.empty())
					return;
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using texture: " + n);

				for (auto &it : replacements)
				{
					if (it.name == n && it.type == Type)
					{
						if (config.verbose)
							CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "texture name matches embedded one - replacing it");
						textures.push_back(std::move(it));
						return;
					}
				}

				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "texture name does not match embedded texture - passing just name");
				MeshImportTexture t;
				t.name = n;
				t.type = Type;
				textures.push_back(std::move(t));
			}

			template<aiTextureType AssimpType, MeshImportTextureType CageType>
			bool loadTextureAssimp(const aiMaterial *mat, Textures &textures)
			{
				const uint32 texCount = mat->GetTextureCount(AssimpType);
				if (texCount == 0)
					return false;
				if (texCount > 1)
					CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "material has multiple (" + texCount + ") textures of same type (" + AssimpType + ")");
				aiString texAsName;
				mat->GetTexture(AssimpType, 0, &texAsName);
				String n = texAsName.C_Str();
				if (isPattern(n, "*", "", ""))
				{ // embedded texture
					n = subString(n, 1, cage::m);
					const uint32 idx = toUint32(n);
					CAGE_ASSERT(idx < imp.GetScene()->mNumTextures);
					const aiTexture *tx = imp.GetScene()->mTextures[idx];
					MeshImportTexture t;
					t.name = tx->mFilename.C_Str();
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using embedded texture with original name: " + t.name);
					if (t.name.empty())
						t.name = n;
					t.name = inputFile + "?" + pathReplaceInvalidCharacters(t.name);
					t.type = CageType;
					if (tx->mHeight == 0)
					{ // compressed buffer
						const PointerRange<const char> buff = { (const char *)tx->pcData, (const char *)tx->pcData + tx->mWidth };
						t.images = imageImportBuffer(buff);
					}
					else
					{ // raw data
						const uint32 bytes = tx->mWidth * tx->mHeight * 4;
						const PointerRange<const char> buff = { (const char *)tx->pcData, (const char *)tx->pcData + bytes };
						ImageImportPart part;
						part.image = newImage();
						part.image->importRaw(buff, Vec2i(tx->mWidth, tx->mHeight), 4, ImageFormatEnum::U8);
						PointerRangeHolder<ImageImportPart> parts;
						parts.push_back(std::move(part));
						t.images.parts = std::move(parts);
					}
					textures.push_back(std::move(t));
				}
				else
				{ // external texture
					if (isPattern(n, "//", "", ""))
						n = String() + "./" + subString(n, 2, cage::m);
					n = pathJoin(pathExtractDirectory(inputFile), n);
					n = pathToAbs(n);
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using texture: " + n);
					MeshImportTexture t;
					t.name = n;
					t.type = CageType;
					textures.push_back(std::move(t));
				}
				return true;
			}

			void finishMaterial(MeshImportPart &part)
			{
				if (none(part.renderFlags & MeshRenderFlags::Lighting))
					part.material.lighting[0] = 0;
				if (any(part.renderFlags & MeshRenderFlags::Fade))
					part.material.lighting[1] = 1;
			}

			void loadMaterialCage(const String &path, MeshImportPart &part)
			{
				CAGE_LOG(SeverityEnum::Info, "meshImport", "overriding material with cage (.cpm) file");
				CAGE_ASSERT(pathIsAbs(path));
				ioSystem.paths.insert(path);

				privat::meshImportConvertToCageFormats(part);

				if (config.verbose)
				{
					CAGE_LOG(SeverityEnum::Info, "meshImport", "available textures:");
					for (const auto &it : part.textures)
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "meshImport", Stringizer() + "texture: " + it.name + ", type: " + meshImportTextureTypeToString(it.type) + ", parts: " + it.images.parts.size());
				}

				{ // reset material to default
					part.material = MeshImportMaterial();
					part.renderFlags = MeshRenderFlags::Default;
					part.renderLayer = 0;
					part.shaderName = {};
				}

				Holder<Ini> ini = newIni();
				ini->importFile(path);

				part.material.albedoBase = Vec4(colorGammaToLinear(Vec3::parse(ini->getString("base", "albedo", "0, 0, 0"))) * ini->getFloat("base", "intensity", 1), ini->getFloat("base", "opacity", 0));
				part.material.specialBase = Vec4(ini->getFloat("base", "roughness", 0), ini->getFloat("base", "metallic", 0), ini->getFloat("base", "emission", 0), ini->getFloat("base", "mask", 0));
				part.material.albedoMult = Vec4(Vec3::parse(ini->getString("mult", "albedo", "1, 1, 1")) * ini->getFloat("mult", "intensity", 1), ini->getFloat("mult", "opacity", 1));
				part.material.specialMult = Vec4(ini->getFloat("mult", "roughness", 1), ini->getFloat("mult", "metallic", 1), ini->getFloat("mult", "emission", 1), ini->getFloat("mult", "mask", 1));

				Textures textures;
				const String pathBase = pathExtractDirectory(path);
				loadTextureCage<MeshImportTextureType::Albedo>(pathBase, +ini, textures, part.textures);
				loadTextureCage<MeshImportTextureType::Special>(pathBase, +ini, textures, part.textures);
				loadTextureCage<MeshImportTextureType::Normal>(pathBase, +ini, textures, part.textures);
				loadTextureCage<MeshImportTextureType::Custom>(pathBase, +ini, textures, part.textures);
				part.textures = std::move(textures);

				for (const String &n : ini->items("flags"))
				{
					const String v = ini->getString("flags", n);
					if (v == "cutOut")
					{
						part.renderFlags |= MeshRenderFlags::CutOut;
						continue;
					}
					if (v == "transparent")
					{
						part.renderFlags |= MeshRenderFlags::Transparent;
						continue;
					}
					if (v == "fade")
					{
						part.renderFlags |= MeshRenderFlags::Fade;
						continue;
					}
					if (v == "twoSided")
					{
						part.renderFlags |= MeshRenderFlags::TwoSided;
						continue;
					}
					if (v == "noDepthTest")
					{
						part.renderFlags &= ~MeshRenderFlags::DepthTest;
						continue;
					}
					if (v == "noDepthWrite")
					{
						part.renderFlags &= ~MeshRenderFlags::DepthWrite;
						continue;
					}
					if (v == "noLighting")
					{
						part.renderFlags &= ~MeshRenderFlags::Lighting;
						continue;
					}
					if (v == "noShadowCast")
					{
						part.renderFlags &= ~MeshRenderFlags::ShadowCast;
						continue;
					}
					if (v == "noCulling")
					{
						part.boundingBox = Aabb::Universe();
						continue;
					}
					CAGE_LOG_THROW(Stringizer() + "provided flag: " + v);
					CAGE_THROW_ERROR(Exception, "unknown material flag");
				}

				part.shaderName = convertPath(pathBase, ini->getString("render", "shader"));
				if (config.verbose && !part.shaderName.empty())
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using shader: " + part.shaderName);

				part.renderLayer = ini->getSint32("render", "layer", 0);
				if (config.verbose && part.renderLayer != 0)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "render layer: " + part.renderLayer);

				part.material.animation[0] = ini->getFloat("animation", "duration", 1);
				part.material.animation[1] = ini->getBool("animation", "loop", true);

				ini->checkUnused();

				finishMaterial(part);
			}

			void loadMaterialAssimp(const uint32 materialIndex, MeshImportPart &part)
			{
				const aiScene *scene = imp.GetScene();
				CAGE_ASSERT(materialIndex < scene->mNumMaterials);
				const aiMaterial *mat = scene->mMaterials[materialIndex];
				if (!mat)
					CAGE_THROW_ERROR(Exception, "material is null");

				part.renderFlags = MeshRenderFlags::Default;

				Textures textures;

				// roughness/metallic textures
				loadTextureAssimp<aiTextureType_GLTF_METALLIC_ROUGHNESS, MeshImportTextureType::GltfPbr>(mat, textures);
				loadTextureAssimp<aiTextureType_METALNESS, MeshImportTextureType::Metallic>(mat, textures);
				loadTextureAssimp<aiTextureType_DIFFUSE_ROUGHNESS, MeshImportTextureType::Roughness>(mat, textures);
				loadTextureAssimp<aiTextureType_SPECULAR, MeshImportTextureType::Specular>(mat, textures);
				loadTextureAssimp<aiTextureType_SHININESS, MeshImportTextureType::Shininess>(mat, textures);

				// factors
				{
					Real rf = Real::Nan(), mf = Real::Nan();
					mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, rf.value);
					mat->Get(AI_MATKEY_METALLIC_FACTOR, mf.value);
					if (valid(rf) || valid(mf))
					{
						Vec4 &t = textures.empty() ? part.material.specialBase : part.material.specialMult;
						if (valid(rf))
							t[0] = rf;
						if (valid(mf))
							t[1] = mf;
					}
					else
					{
						aiColor3D spec = aiColor3D(0);
						if (mat->Get(AI_MATKEY_COLOR_SPECULAR, spec))
						{ // convert specular color to roughness/metallic material
							const Vec2 s = colorSpecularToRoughnessMetallic(conv(spec));
							part.material.specialBase[0] = s[0];
							part.material.specialBase[1] = s[1];
						}
						else
						{ // convert shininess to roughness
							float shininess = -1;
							mat->Get(AI_MATKEY_SHININESS, shininess);
							if (shininess >= 0)
								part.material.specialBase[0] = sqrt(2 / (2 + shininess));
						}
					}
				}

				// two sided
				{
					int twosided = false;
					mat->Get(AI_MATKEY_TWOSIDED, twosided);
					if (twosided)
						part.renderFlags |= MeshRenderFlags::TwoSided;
				}

				// opacity
				if (loadTextureAssimp<aiTextureType_OPACITY, MeshImportTextureType::Opacity>(mat, textures))
				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "enabling transparent flag due to opacity texture");
					part.renderFlags |= MeshRenderFlags::Transparent;
				}
				Real opacity = 1;
				mat->Get(AI_MATKEY_OPACITY, opacity.value);
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "assimp loaded opacity: " + opacity);
				if (opacity == 1)
				{
					Real transmission = 0;
					mat->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission.value);
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "assimp loaded transmission: " + transmission);
					if (transmission > 0)
						opacity = 1 - transmission * 0.5;
				}
				if (opacity > 0 && opacity < 1)
				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "enabling transparent flag due to opacity");
					part.renderFlags |= MeshRenderFlags::Transparent;
				}
				aiString gltfAlphaMode;
				mat->Get(AI_MATKEY_GLTF_ALPHAMODE, gltfAlphaMode);
				if (config.verbose && gltfAlphaMode != aiString(""))
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "assimp loaded gltf alpha mode: " + gltfAlphaMode.C_Str());
				if (gltfAlphaMode == aiString("BLEND"))
					part.renderFlags |= MeshRenderFlags::Transparent;
				if (gltfAlphaMode == aiString("MASK"))
					part.renderFlags |= MeshRenderFlags::CutOut;

				// albedo
				const auto &albedoCheckAlpha = [&](aiTextureType type)
				{
					int flg = 0;
					mat->Get(AI_MATKEY_TEXFLAGS(type, 0), flg);
					if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha && opacity > 0)
					{
						if (config.verbose)
							CAGE_LOG(SeverityEnum::Info, "meshImport", "enabling transparent flag due to albedo texture alpha flag");
						part.renderFlags |= MeshRenderFlags::Transparent;
					}
				};
				if (loadTextureAssimp<aiTextureType_BASE_COLOR, MeshImportTextureType::Albedo>(mat, textures))
					albedoCheckAlpha(aiTextureType_BASE_COLOR);
				else if (loadTextureAssimp<aiTextureType_DIFFUSE, MeshImportTextureType::Albedo>(mat, textures))
					albedoCheckAlpha(aiTextureType_DIFFUSE);
				else
				{
					aiColor3D color = aiColor3D(1);
					const auto &getColor = [&](const char *a, int b, int c) // temporary fix until https://github.com/assimp/assimp/pull/5161 is merged
					{
						aiColor4D tmp;
						if (mat->Get(a, b, c, tmp) == aiReturn_SUCCESS)
							color = aiColor3D(tmp.r, tmp.g, tmp.b);
					};
					getColor(AI_MATKEY_COLOR_DIFFUSE);
					getColor(AI_MATKEY_BASE_COLOR);
					part.material.albedoBase = Vec4(colorGammaToLinear(conv(color)), part.material.albedoBase[3]);
				}

				if (opacity > 0)
					part.material.albedoMult[3] = opacity;

				// bump/normal map
				loadTextureAssimp<aiTextureType_HEIGHT, MeshImportTextureType::Bump>(mat, textures);
				loadTextureAssimp<aiTextureType_DISPLACEMENT, MeshImportTextureType::Bump>(mat, textures);
				loadTextureAssimp<aiTextureType_NORMALS, MeshImportTextureType::Normal>(mat, textures);
				loadTextureAssimp<aiTextureType_NORMAL_CAMERA, MeshImportTextureType::Normal>(mat, textures);

				// extra textures
				loadTextureAssimp<aiTextureType_AMBIENT_OCCLUSION, MeshImportTextureType::AmbientOcclusion>(mat, textures);
				loadTextureAssimp<aiTextureType_ANISOTROPY, MeshImportTextureType::Anisotropy>(mat, textures);
				loadTextureAssimp<aiTextureType_CLEARCOAT, MeshImportTextureType::Clearcoat>(mat, textures);
				loadTextureAssimp<aiTextureType_EMISSION_COLOR, MeshImportTextureType::Emission>(mat, textures);
				loadTextureAssimp<aiTextureType_EMISSIVE, MeshImportTextureType::Emission>(mat, textures);
				loadTextureAssimp<aiTextureType_SHEEN, MeshImportTextureType::Sheen>(mat, textures);
				loadTextureAssimp<aiTextureType_TRANSMISSION, MeshImportTextureType::Transmission>(mat, textures);

				part.textures = std::move(textures);

				finishMaterial(part);
			}

			void loadMaterial(const uint32 materialIndex, MeshImportPart &part)
			{
				loadMaterialAssimp(materialIndex, part);

				String path = config.materialPathOverride;
				if (!path.empty())
				{
					path = pathJoin(pathExtractDirectory(inputFile), path);
					loadMaterialCage(path, part);
					return;
				}

				path = inputFile;
				{
					aiMaterial *m = imp.GetScene()->mMaterials[materialIndex];
					if (m && m->GetName().length)
						path += String() + "_" + m->GetName().C_Str();
					else if (!config.materialNameAlternative.empty())
						path += String() + "_" + config.materialNameAlternative;
				}
				path += ".cpm";
				path = pathToAbs(path);

				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "looking for implicit material at " + path);

				if (pathIsFile(path))
					loadMaterialCage(path, part);
			}

			const String inputFile;
			const MeshImportConfig config;
			CageIoSystem ioSystem;
			Assimp::Importer imp;
			Holder<AssimpSkeleton> skeleton;

			Vec3 fixUnitVector(const char *name, Vec3 n) const
			{
				if (abs(lengthSquared(n) - 1) > 1e-3)
				{
					CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "fixing denormalized " + name + ": " + n);
					n = normalize(n);
				}
				if (!n.valid())
				{
					if (config.passInvalidVectors)
					{
						CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "passing invalid " + name + ": " + n);
						n = Vec3();
					}
					else
					{
						CAGE_LOG_THROW(Stringizer() + "vector " + name + ": " + n);
						CAGE_THROW_ERROR(Exception, "invalid vector");
					}
				}
				return n;
			}
		};
	}

	MeshImportResult meshImportFiles(const String &filename, const MeshImportConfig &config)
	{
		AssimpContext context(pathToAbs(filename), config);
		const aiScene *scene = context.imp.GetScene();

		MeshImportResult result;

		{
			PointerRangeHolder<MeshImportPart> parts;
			for (uint32 i = 0; i < scene->mNumMeshes; i++)
			{
				const aiMesh *msh = scene->mMeshes[i];
				MeshImportPart p;
				p.objectName = convStrTruncate(msh->mName);
				p.materialName = convStrTruncate(scene->mMaterials[msh->mMaterialIndex]->GetName());
				p.mesh = context.mesh(i);
				p.boundingBox = p.mesh->boundingBox();
				context.loadMaterial(msh->mMaterialIndex, p);
				parts.push_back(std::move(p));
			}
			result.parts = std::move(parts);
		}

		if (config.verticalFlipUv)
		{
			for (MeshImportPart &part : result.parts)
			{
				for (Vec2 &v : part.mesh->uvs())
					v[1] = 1 - v[1];
				//for (Vec3 &v : part.mesh->uvs3()) // uvs3 are vectors in range -1..1, do not invert
				//	v[1] = 1 - v[1];
			}
		}

		if (context.skeleton)
		{
			result.skeleton = context.skeletonRig();
			PointerRangeHolder<MeshImportAnimation> anims;
			for (uint32 i = 0; i < scene->mNumAnimations; i++)
			{
				const aiAnimation *ani = scene->mAnimations[i];
				if (ani->mNumChannels == 0 || ani->mNumMeshChannels != 0 || ani->mNumMorphMeshChannels != 0)
					continue;
				MeshImportAnimation a;
				a.name = convStrTruncate(ani->mName);
				a.animation = context.animation(i);
				anims.push_back(std::move(a));
			}
			result.animations = std::move(anims);

			// extend bounding boxes to contain all animations
			for (MeshImportPart &part : result.parts)
			{
				if (!part.mesh->boneIndices().empty())
				{
					for (const MeshImportAnimation &ani : result.animations)
					{
						for (Real t = 0; t <= 1; t += 0.02) // sample the animation at 50 positions
						{
							Holder<Mesh> tmp = part.mesh->copy();
							animateMesh(+result.skeleton, +ani.animation, t, +tmp);
							Aabb box = tmp->boundingBox();
							box = enlarge(box);
							part.boundingBox += box;
						}
					}
				}
			}
		}

		{
			PointerRangeHolder<String> pths;
			for (const String &p : context.ioSystem.paths)
			{
				CAGE_ASSERT(pathIsAbs(p));
				pths.push_back(p);
			}
			result.paths = std::move(pths);
		}

		return result;
	}
}
