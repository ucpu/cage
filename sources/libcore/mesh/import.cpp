#include <cage-core/ini.h>
#include <cage-core/color.h>
#include <cage-core/files.h>
#include <cage-core/string.h>
#include <cage-core/image.h>
#include <cage-core/mesh.h>
#include <cage-core/meshImport.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/pointerRangeHolder.h>

#include <assimp/scene.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

#include <set>
#include <map>
#include <vector>

namespace cage
{
	namespace
	{
		class CageLogStream : public Assimp::LogStream
		{
		public:
			const cage::SeverityEnum severity;

			CageLogStream(cage::SeverityEnum severity) : severity(severity)
			{}

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

		Vec4 conv(const aiColor4D &v)
		{
			static_assert(sizeof(aiColor4D) == sizeof(cage::Vec4));
			return *(Vec4 *)&v;
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

		Mat3 axesMatrix(const detail::StringBase<6> axes)
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

		String convStrTruncate(const aiString &str, uint32 maxLen = cage::String::MaxLength / 2)
		{
			String s;
			s.rawLength() = min(str.length, maxLen);
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
			case aiPrimitiveType_POINT: return 1;
			case aiPrimitiveType_LINE: return 2;
			case aiPrimitiveType_TRIANGLE: return 3;
			default: CAGE_THROW_ERROR(Exception, "mesh has invalid primitive type");
			}
		}

		class CageIoStream : public Assimp::IOStream
		{
		public:
			explicit CageIoStream(cage::Holder<cage::File> r) : r(std::move(r))
			{}

			virtual ~CageIoStream()
			{}

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

			size_t Write(const void *pvBuffer, size_t pSize, size_t pCount) override
			{
				CAGE_THROW_ERROR(NotImplemented, "cageIOStream::Write");
			}

			aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override
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

			size_t Tell() const override
			{
				return (size_t)r->tell();
			}

			size_t FileSize() const override
			{
				return (size_t)r->size();
			}

			void Flush() override
			{
				CAGE_THROW_ERROR(NotImplemented, "cageIOStream::Flush");
			}

		private:
			cage::Holder<cage::File> r;
		};

		class CageIoSystem : public Assimp::IOSystem
		{
		public:
			bool Exists(const char *pFile) const override
			{
				return any(pathType(pathToAbs(pFile)) & PathTypeFlags::File);
			}

			char getOsSeparator() const override
			{
				return '/';
			}

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

			void Close(Assimp::IOStream *pFile) override
			{
				delete (CageIoStream *)pFile;
			}

			bool ComparePaths(const char *one, const char *second) const override
			{
				CAGE_THROW_ERROR(NotImplemented, "cageIOsystem::ComparePaths");
			}

			std::set<String> paths;
		};

		struct CmpAiStr
		{
			bool operator ()(const aiString &a, const aiString &b) const
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
						CAGE_LOG_DEBUG(SeverityEnum::Warning, "assimp", Stringizer() + "renamed a node: '" + n->mName.C_Str() + "'");
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

			aiNode *node(aiBone *bone) const
			{
				return node(index(bone));
			}

			aiNode *node(uint16 index) const
			{
				if (index == m)
					return nullptr;
				return nodes[index];
			}

			aiBone *bone(aiNode *node) const
			{
				return bone(index(node));
			}

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

			aiBone *parent(aiBone *bone_) const
			{
				return bone(parent(index(bone_)));
			}

			// this may skip some nodes in the original hierarchy - this function returns node that corresponds to another bone
			aiNode *parent(aiNode *node_) const
			{
				return node(parent(index(node_)));
			}

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

				//imp.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, true);
				imp.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 30);

				try
				{
					static constexpr uint32 AssimpDefaultLoadFlags =
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
						//aiProcess_SplitLargeModeles |
						0;

					static constexpr uint32 AssimpBakeLoadFlags =
						aiProcess_RemoveRedundantMaterials |
						//aiProcess_FindInstances |
						aiProcess_OptimizeMeshes |
						aiProcess_OptimizeGraph |
						0;

					uint32 flags = AssimpDefaultLoadFlags;
					if (config.mergeParts)
						flags |= AssimpBakeLoadFlags;
					if (config.generateNormals)
						flags |= aiProcess_GenSmoothNormals;
					if (config.generateTangents)
						flags |= aiProcess_CalcTangentSpace;
					if (config.trianglesOnly)
						imp.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE | aiPrimitiveType_POLYGON);

					imp.SetIOHandler(&this->ioSystem);
					if (!imp.ReadFile(inputFile.c_str(), flags))
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

				if (!scene)
					CAGE_THROW_ERROR(Exception, "scene is null");
				if ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == AI_SCENE_FLAGS_INCOMPLETE)
					CAGE_THROW_ERROR(Exception, "the scene is incomplete");

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
						if (am->HasTangentsAndBitangents())
							contains += "tangents ";
						if (am->HasVertexColors(0))
							contains += "colors ";
						CAGE_LOG_CONTINUE(SeverityEnum::Note, "meshImport", Stringizer() + "index: " + i + ", object: '" + objname + "', material: '" + matname + "', contains: " + contains);
					}

					// print animations
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "found " + scene->mNumAnimations + " animations");
					for (uint32 i = 0; i < scene->mNumAnimations; i++)
					{
						const aiAnimation *ani = scene->mAnimations[i];
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "meshImport", Stringizer() + "index: " + i + ", animation: '" + convStrTruncate(ani->mName) + "', channels: " + ani->mNumChannels);
					}

					if (axesScale != Mat3())
						CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "using axes/scale conversion matrix: " + axesScale);
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

				const Mat4 axesScale = Mat4(this->axesScale);
				const Mat4 axesScaleInv = inverse(axesScale);

				const Mat4 globalInverse = inverse(conv(imp.GetScene()->mRootNode->mTransformation)) * axesScale;
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
					const Mat4 o = (b ? conv(b->mOffsetMatrix) : Mat4()) * axesScaleInv;
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
						CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "channel index: " + channelIndex + ", name: '" + n->mNodeName.data + "', has no corresponding bone and will be ignored");
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
				const uint32 animationBonesCount = numeric_cast<uint32>(bones.size());
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
					for (uint16 i = 0; i < bones.size(); i++)
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
				case 1: poly->type(MeshTypeEnum::Points); break;
				case 2: poly->type(MeshTypeEnum::Lines); break;
				case 3: poly->type(MeshTypeEnum::Triangles); break;
				default: CAGE_THROW_CRITICAL(Exception, "invalid mesh type enum");
				}

				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying vertex positions");
					std::vector<Vec3> ps;
					ps.reserve(verticesCount);
					for (uint32 i = 0; i < verticesCount; i++)
					{
						Vec3 p = axesScale * conv(am->mVertices[i]);
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
						Vec3 n = axes * conv(am->mNormals[i]);
						static constexpr const char Name[] = "normal";
						ps.push_back(fixUnitVector<Name>(n));
					}
					poly->normals(ps);
				}

				if (am->HasTangentsAndBitangents())
				{
					if (config.verbose)
						CAGE_LOG(SeverityEnum::Info, "meshImport", "copying tangents");
					std::vector<Vec3> ts, bs;
					ts.reserve(verticesCount);
					bs.reserve(verticesCount);
					for (uint32 i = 0; i < verticesCount; i++)
					{
						Vec3 n = axes * conv(am->mTangents[i]);
						static constexpr const char Name[] = "tangent";
						ts.push_back(fixUnitVector<Name>(n));
					}
					for (uint32 i = 0; i < verticesCount; i++)
					{
						Vec3 n = axes * conv(am->mBitangents[i]);
						static constexpr const char Name[] = "bitangent";
						bs.push_back(fixUnitVector<Name>(n));
					}
					poly->tangents(ts);
					poly->bitangents(bs);
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
			static constexpr MeshImportTextureType RoughTex = (MeshImportTextureType)1001;
			static constexpr MeshImportTextureType MetalTex = (MeshImportTextureType)1002;
			static constexpr MeshImportTextureType RoughMetalBgTex = (MeshImportTextureType)1003;

			static void pickChannel(MeshImportTexture &textures, uint32 channel)
			{
				for (auto &it : textures.images.parts)
				{
					auto vals = imageChannelsSplit(+it.image);
					if (channel >= vals.size())
						CAGE_THROW_ERROR(Exception, "channel index out of range");
					it.image = std::move(vals[channel]);
				}
				textures.name += Stringizer() + "_ch" + channel;
			}

			static MeshImportTexture duplicateChannel(MeshImportTexture &textures, uint32 channel)
			{
				PointerRangeHolder<ImageImportPart> parts;
				for (auto &it : textures.images.parts)
				{
					auto vals = imageChannelsSplit(+it.image);
					if (channel >= vals.size())
						CAGE_THROW_ERROR(Exception, "channel index out of range");
					ImageImportPart p;
					p.image = std::move(vals[channel]);
					parts.push_back(std::move(p));
				}
				MeshImportTexture result;
				result.name = Stringizer() + textures.name + "_ch" + channel;
				result.images.parts = std::move(parts);
				return result;
			}

			void convertSpecialTextures(Textures &input, Textures &output)
			{
				CAGE_ASSERT(!input.empty());

				for (auto &it : input)
					imageImportConvertRawToImages(it.images);

				// pick correct channels for roughness or metallic textures
				for (auto &it : input)
				{
					switch (it.type)
					{
					case RoughTex:
					{
						switch (it.images.parts[0].image->channels())
						{
						case 1: break;
						case 4: pickChannel(it, 3); break;
						default: CAGE_THROW_ERROR(Exception, "unexpected channels count for roughness texture");
						}
					} break;
					case MetalTex:
					{
						switch (it.images.parts[0].image->channels())
						{
						case 1: break;
							// todo find which channel contains metallic data
						default: CAGE_THROW_ERROR(Exception, "unexpected channels count for metallic texture");
						}
					} break;
					case RoughMetalBgTex:
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid type of texture for combining into special texture");
					}
				}

				// split roughness-metallic texture
				if (input.size() == 1 && input[0].type == RoughMetalBgTex)
				{
					CAGE_ASSERT(input.size() == 1);
					MeshImportTexture rt = duplicateChannel(input[0], 1);
					MeshImportTexture mt = duplicateChannel(input[0], 2);
					rt.type = RoughTex;
					mt.type = MetalTex;
					input.clear();
					input.push_back(std::move(rt));
					input.push_back(std::move(mt));
				}

				// early exit for roughness-only texture
				if (input.size() == 1 && input[0].type == RoughTex)
				{
					CAGE_ASSERT(input[0].images.parts.size() == 1);
					CAGE_ASSERT(input[0].images.parts[0].image->channels() == 1);
					input[0].type = MeshImportTextureType::Special;
					output.push_back(std::move(input[0]));
					return;
				}

				// early exit for metallic-only texture
				if (input.size() == 1 && input[0].type == MetalTex)
				{
					CAGE_ASSERT(input[0].images.parts.size() == 1);
					CAGE_ASSERT(input[0].images.parts[0].image->channels() == 1);
					Holder<Image> arr[2];
					arr[1] = std::move(input[0].images.parts[0].image);
					input[0].images.parts[0].image = imageChannelsJoin(arr);
					input[0].type = MeshImportTextureType::Special;
					output.push_back(std::move(input[0]));
					return;
				}

				// combine special texture from both sources
				if (input.size() == 2 && input[0].type == RoughTex && input[1].type == MetalTex)
				{
					CAGE_ASSERT(input[0].images.parts.size() == 1);
					CAGE_ASSERT(input[1].images.parts.size() == 1);
					CAGE_ASSERT(input[0].images.parts[0].image->channels() == 1);
					CAGE_ASSERT(input[1].images.parts[0].image->channels() == 1);
					Holder<Image> arr[2];
					arr[0] = std::move(input[0].images.parts[0].image);
					arr[1] = std::move(input[1].images.parts[0].image);
					ImageImportPart part;
					part.image = imageChannelsJoin(arr);
					PointerRangeHolder<ImageImportPart> parts;
					parts.push_back(std::move(part));
					MeshImportTexture res;
					res.images.parts = std::move(parts);
					const String base = split(input[0].name, "?");
					split(input[1].name, "?");
					res.name = Stringizer() + base + "?special_" + input[0].name + "_" + input[1].name;
					res.type = MeshImportTextureType::Special;
					output.push_back(std::move(res));
					return;
				}

				CAGE_THROW_CRITICAL(Exception, "impossible combination of roughness metallic textures for combining into special texture");
			}

			String convertPath(const String &pathBase, String n) const
			{
				if (n.empty())
					return n;
				if (n[0] == '/')
					n = pathJoin(config.rootPath, trim(n, true, false, "/"));
				else
					n = pathJoinUnchecked(pathBase, n);
				n = pathToAbs(n);
				return n;
			}

			template<MeshImportTextureType Type>
			void loadTextureCage(const String &pathBase, Ini *ini, Textures &textures)
			{
				static constexpr const char *names[] = {
					"",
					"albedo",
					"special",
					"normal",
				};
				String n = ini->getString("textures", names[(uint32)Type]);
				n = convertPath(pathBase, n);
				if (n.empty())
					return;
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using texture: " + n);
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

			void loadMaterialCage(const String &path, MeshImportPart &part)
			{
				CAGE_LOG(SeverityEnum::Info, "meshImport", "using cage (.cpm) material");
				CAGE_ASSERT(pathIsAbs(path));
				ioSystem.paths.insert(path);

				Holder<Ini> ini = newIni();
				ini->importFile(path);

				part.material.albedoBase = Vec4(
					colorGammaToLinear(Vec3::parse(ini->getString("base", "albedo", "0, 0, 0"))) * ini->getFloat("base", "intensity", 1),
					ini->getFloat("base", "opacity", 0)
				);

				part.material.specialBase = Vec4(
					ini->getFloat("base", "roughness", 0),
					ini->getFloat("base", "metallic", 0),
					ini->getFloat("base", "emission", 0),
					ini->getFloat("base", "mask", 0)
				);

				part.material.albedoMult = Vec4(
					colorGammaToLinear(Vec3::parse(ini->getString("mult", "albedo", "1, 1, 1"))) * ini->getFloat("mult", "intensity", 1),
					ini->getFloat("mult", "opacity", 1)
				);

				part.material.specialMult = Vec4(
					ini->getFloat("mult", "roughness", 1),
					ini->getFloat("mult", "metallic", 1),
					ini->getFloat("mult", "emission", 1),
					ini->getFloat("mult", "mask", 1)
				);

				Textures textures;
				const String pathBase = pathExtractDirectory(path);
				loadTextureCage<MeshImportTextureType::Albedo>(pathBase, +ini, textures);
				loadTextureCage<MeshImportTextureType::Special>(pathBase, +ini, textures);
				loadTextureCage<MeshImportTextureType::Normal>(pathBase, +ini, textures);
				part.textures = std::move(textures);

				for (const String &n : ini->items("flags"))
				{
					const String v = ini->getString("flags", n);
					if (v == "translucent")
					{
						part.renderFlags |= MeshRenderFlags::Translucent;
						continue;
					}
					if (v == "alphaClip")
					{
						part.renderFlags |= MeshRenderFlags::AlphaClip;
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
					CAGE_LOG_THROW(Stringizer() + "provided flag: '" + v + "'");
					CAGE_THROW_ERROR(Exception, "unknown material flag");
				}

				part.shaderDepthName = convertPath(pathBase, ini->getString("shaders", "depth"));
				if (config.verbose && !part.shaderDepthName.empty())
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using depth shader: " + part.shaderDepthName);
				part.shaderColorName = convertPath(pathBase, ini->getString("shaders", "color"));
				if (config.verbose && !part.shaderColorName.empty())
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "using color shader: " + part.shaderColorName);

				part.renderLayer = ini->getSint32("shaders", "layer", 0);
				if (config.verbose && part.renderLayer != 0)
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "render layer: " + part.renderLayer);

				ini->checkUnused();
			}

			void loadMaterialAssimp(const uint32 materialIndex, MeshImportPart &part)
			{
				if (config.verbose)
					CAGE_LOG(SeverityEnum::Info, "meshImport", "converting assimp material");

				const aiScene *scene = imp.GetScene();
				CAGE_ASSERT(materialIndex < scene->mNumMaterials);
				const aiMaterial *mat = scene->mMaterials[materialIndex];
				if (!mat)
					CAGE_THROW_ERROR(Exception, "material is null");

				Textures textures;

				// opacity
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
						CAGE_LOG(SeverityEnum::Info, "meshImport", "enabling translucent flag due to opacity");
					part.renderFlags |= MeshRenderFlags::Translucent;
				}

				// albedo
				const auto &albedoCheckAlpha = [&](aiTextureType type) {
					int flg = 0;
					mat->Get(AI_MATKEY_TEXFLAGS(type, 0), flg);
					if ((flg & aiTextureFlags_UseAlpha) == aiTextureFlags_UseAlpha && opacity > 0)
					{
						if (config.verbose)
							CAGE_LOG(SeverityEnum::Info, "meshImport", "enabling translucent flag due to albedo texture flag");
						part.renderFlags |= MeshRenderFlags::Translucent;
					}
				};
				if (loadTextureAssimp<aiTextureType_BASE_COLOR, MeshImportTextureType::Albedo>(mat, textures))
				{
					albedoCheckAlpha(aiTextureType_BASE_COLOR);
				}
				else if (loadTextureAssimp<aiTextureType_DIFFUSE, MeshImportTextureType::Albedo>(mat, textures))
				{
					albedoCheckAlpha(aiTextureType_DIFFUSE);
				}
				else
				{
					aiColor3D color = aiColor3D(1);
					mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
					part.material.albedoBase = Vec4(colorGammaToLinear(conv(color)), part.material.albedoBase[3]);
				}

				if (opacity > 0)
					part.material.albedoMult[3] = opacity;

				// special
				const auto &specialCheckFactors = [&]() {
					Real rf = 1, mf = 1;
					mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, rf.value);
					mat->Get(AI_MATKEY_METALLIC_FACTOR, mf.value);
					part.material.specialMult[0] = rf;
					part.material.specialMult[1] = mf;
				};
				if (mat->GetTextureCount(aiTextureType_UNKNOWN) > 0)
				{
					Textures texs; // temporary textures before merging
					loadTextureAssimp<aiTextureType_UNKNOWN, RoughMetalBgTex>(mat, texs);
					convertSpecialTextures(texs, textures);
					specialCheckFactors();
				}
				else if (mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) + mat->GetTextureCount(aiTextureType_METALNESS) > 0)
				{
					Textures texs; // temporary textures before merging
					loadTextureAssimp<aiTextureType_DIFFUSE_ROUGHNESS, RoughTex>(mat, texs);
					loadTextureAssimp<aiTextureType_METALNESS, MetalTex>(mat, texs);
					convertSpecialTextures(texs, textures);
					specialCheckFactors();
				}
				else if (!loadTextureAssimp<aiTextureType_SPECULAR, MeshImportTextureType::Special>(mat, textures))
				{
					aiColor3D spec = aiColor3D(0);
					mat->Get(AI_MATKEY_COLOR_SPECULAR, spec);
					if (conv(spec) != Vec3(0))
					{ // convert specular color to special material
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

				// normal map
				loadTextureAssimp<aiTextureType_HEIGHT, MeshImportTextureType::Normal>(mat, textures);
				loadTextureAssimp<aiTextureType_NORMALS, MeshImportTextureType::Normal>(mat, textures);

				// two sided
				{
					int twosided = false;
					mat->Get(AI_MATKEY_TWOSIDED, twosided);
					if (twosided)
						part.renderFlags |= MeshRenderFlags::TwoSided;
				}

				part.textures = std::move(textures);
			}

			void loadMaterial(const uint32 materialIndex, MeshImportPart &part)
			{
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
					CAGE_LOG(SeverityEnum::Info, "meshImport", Stringizer() + "looking for implicit material at '" + path + "'");

				if (pathIsFile(path))
				{
					loadMaterialCage(path, part);
					return;
				}

				loadMaterialAssimp(materialIndex, part);
			}

			const String inputFile;
			const MeshImportConfig config;
			const Mat3 axes = axesMatrix(config.axesSwizzle);
			const Mat3 axesScale = axes * config.scale;
			CageIoSystem ioSystem;
			Assimp::Importer imp;
			Holder<AssimpSkeleton> skeleton;

			template<const char Name[]>
			Vec3 fixUnitVector(Vec3 n) const
			{
				if (abs(lengthSquared(n) - 1) > 1e-3)
				{
					CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "fixing denormalized " + Name + ": " + n);
					n = normalize(n);
				}
				if (!n.valid())
				{
					if (config.passInvalidVectors)
					{
						CAGE_LOG(SeverityEnum::Warning, "meshImport", Stringizer() + "passing invalid " + Name + ": " + n);
						n = Vec3();
					}
					else
						CAGE_THROW_ERROR(Exception, "invalid vector");
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
				p.renderFlags = MeshRenderFlags::DepthTest | MeshRenderFlags::DepthWrite | MeshRenderFlags::Lighting | MeshRenderFlags::ShadowCast;
				context.loadMaterial(msh->mMaterialIndex, p);
				parts.push_back(std::move(p));
			}
			result.parts = std::move(parts);
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

	namespace detail
	{
		StringLiteral meshImportTextureTypeToString(MeshImportTextureType type)
		{
			switch (type)
			{
			case MeshImportTextureType::None: return "none";
			case MeshImportTextureType::Albedo: return "albedo";
			case MeshImportTextureType::Special: return "special";
			case MeshImportTextureType::Normal: return "normal";
			default: return "unknown";
			}
		}
	}
}
