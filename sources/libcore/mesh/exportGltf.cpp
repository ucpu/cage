#include <cage-core/files.h>
#include <cage-core/string.h>
#include <cage-core/image.h>
#include <cage-core/meshExport.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/geometry.h>
#include <cage-core/tasks.h>
#include <vector>
#include <string>

namespace cage
{
	namespace
	{
		Holder<PointerRange<char>> exportImpl(const String &filename, const MeshExportGltfConfig &config)
		{
			CAGE_ASSERT(config.mesh);
			if (config.mesh->type() != MeshTypeEnum::Triangles)
				CAGE_THROW_ERROR(Exception, "exporting gltf requires triangle mesh");
			if (config.mesh->verticesCount() > 0 && config.mesh->indicesCount() == 0)
				CAGE_THROW_ERROR(Exception, "exporting gltf requires indexed mesh");

			// prepare textures

			if (filename.empty())
			{
				const auto &tex = [&](const MeshExportGltfTexture &t) {
					if (!t.filename.empty())
						CAGE_THROW_ERROR(Exception, "cannot export textures when exporting gltf into buffer only");
				};

				tex(config.albedo);
				tex(config.pbr);
				tex(config.normal);
			}

			// prepare the buffer, views and accessors

			struct View
			{
				// attribute
				detail::StringBase<20> attribute;

				// accessor
				detail::StringBase<50> min;
				detail::StringBase<50> max;
				detail::StringBase<20> type;
				uint32 componentType = 0;
				uint32 count = 0;

				// view
				uint32 byteOffset = 0;
				uint32 byteLength = 0;
				uint32 target = 0;
			};
			std::vector<View> views;
			views.reserve(10);

			uint32 indicesIndex = m;

			MemoryBuffer buffer;
			Serializer ser(buffer);

			{
				View v;
				v.attribute = "POSITION";
				const Aabb box = config.mesh->boundingBox();
				v.min = Stringizer() + "[" + box.a[0] + "," + box.a[1] + "," + box.a[2] + "]";
				v.max = Stringizer() + "[" + box.b[0] + "," + box.b[1] + "," + box.b[2] + "]";
				v.type = "VEC3";
				v.componentType = 5126; // float
				v.count = numeric_cast<uint32>(config.mesh->positions().size());
				v.byteOffset = numeric_cast<uint32>(buffer.size());
				ser.write(bufferCast(config.mesh->positions()));
				v.byteLength = numeric_cast<uint32>(buffer.size()) - v.byteOffset;
				v.target = 34962; // array buffer
				views.push_back(v);
			}

			if (config.mesh->uvs().size() > 0)
			{
				View v;
				v.attribute = "TEXCOORD_0";
				v.type = "VEC2";
				v.componentType = 5126; // float
				v.count = numeric_cast<uint32>(config.mesh->uvs().size());
				v.byteOffset = numeric_cast<uint32>(buffer.size());
				if (config.verticalFlipUv)
				{
					std::vector<Vec2> uvs(config.mesh->uvs().begin(), config.mesh->uvs().end());
					for (Vec2 &uv : uvs)
						uv[1] = 1 - uv[1];
					ser.write(bufferCast(PointerRange<const Vec2>(uvs)));
				}
				else
					ser.write(bufferCast(config.mesh->uvs()));
				v.byteLength = numeric_cast<uint32>(buffer.size()) - v.byteOffset;
				v.target = 34962; // array buffer
				views.push_back(v);
			}

			if (config.mesh->normals().size() > 0)
			{
				View v;
				v.attribute = "NORMAL";
				v.type = "VEC3";
				v.componentType = 5126; // float
				v.count = numeric_cast<uint32>(config.mesh->normals().size());
				v.byteOffset = numeric_cast<uint32>(buffer.size());
				ser.write(bufferCast(config.mesh->normals()));
				v.byteLength = numeric_cast<uint32>(buffer.size()) - v.byteOffset;
				v.target = 34962; // array buffer
				views.push_back(v);
			}

			// todo tangents

			{
				indicesIndex = numeric_cast<uint32>(views.size());
				View v;
				v.type = "SCALAR";
				v.componentType = 5125; // unsigned int
				v.count = numeric_cast<uint32>(config.mesh->indices().size());
				v.byteOffset = numeric_cast<uint32>(buffer.size());
				ser.write(bufferCast(config.mesh->indices()));
				v.byteLength = numeric_cast<uint32>(buffer.size()) - v.byteOffset;
				v.target = 34963; // element array buffer
				views.push_back(v);
			}

			// textures

			struct Texture
			{
				String filename;
				Holder<PointerRange<char>> buff;
				const MeshExportGltfTexture *tex = nullptr;

				void operator () ()
				{
					CAGE_ASSERT(tex);
					if (tex->image && !tex->filename.empty())
					{
						const String pth = pathJoin(pathExtractDirectory(filename), tex->filename);
						tex->image->exportFile(pth);
					}
					else if (tex->image)
						buff = tex->image->exportBuffer();
				}

				uint32 bufferView = m;
				uint32 type = 0; // 1 = albedo, 2 = roughness-metallic, 3 = normal
			};
			std::vector<Texture> textures;

			if (config.albedo.image || !config.albedo.filename.empty())
				textures.push_back(Texture{ .filename = filename, .tex = &config.albedo, .type = 1 });
			if (config.pbr.image || !config.pbr.filename.empty())
				textures.push_back(Texture{ .filename = filename, .tex = &config.pbr, .type = 2 });
			if (config.normal.image || !config.normal.filename.empty())
				textures.push_back(Texture{ .filename = filename, .tex = &config.normal, .type = 3 });

			tasksRunBlocking<Texture>("gltf export texture", textures);

			for (Texture &t : textures)
			{
				if (t.buff)
				{
					t.bufferView = numeric_cast<uint32>(views.size());
					View v;
					v.byteOffset = numeric_cast<uint32>(buffer.size());
					ser.write(bufferCast(*t.buff));
					v.byteLength = numeric_cast<uint32>(buffer.size()) - v.byteOffset;
					while ((buffer.size() % 4) != 0)
						ser << '\0';
					views.push_back(v);
				}
			}

			// prepare the json

			std::string json;
			{
				std::string viewsJson, accessorsJson, attributesJson, texturesJson, imagesJson, materialJson, materialPbrJson;

				uint32 index = 0;
				for (const View &v : views)
				{
					// view
					if (index > 0)
						viewsJson += ",";
					viewsJson += (Stringizer()
						+ "{\"buffer\":0,\"byteOffset\":" + v.byteOffset
						+ ",\"byteLength\":" + v.byteLength
						+ (v.target == 0 ? Stringizer() : (Stringizer() + ",\"target\":" + v.target)) + "}").value.c_str();

					// accessor
					if (v.componentType > 0)
					{
						if (index > 0)
							accessorsJson += ",";
						accessorsJson += (Stringizer()
							+ "{\"bufferView\":" + index
							+ (v.min.empty() ? Stringizer() : (Stringizer() + ",\"min\":" + v.min))
							+ (v.max.empty() ? Stringizer() : (Stringizer() + ",\"max\":" + v.max))
							+ ",\"componentType\":" + v.componentType
							+ ",\"type\":\"" + v.type + "\""
							+ ",\"count\":" + v.count + "}").value.c_str();
					}

					// attribute
					if (!v.attribute.empty())
					{
						if (index > 0)
							attributesJson += ",";
						attributesJson += (Stringizer()
							+ "\"" + v.attribute + "\":" + index).value.c_str();
					}

					index++;
				}

				static constexpr const char *textureTypeNames[4] = {
					"invalid",
					"albedo",
					"pbr",
					"normal",
				};

				index = 0;
				for (const Texture &t : textures)
				{
					if (index > 0)
					{
						texturesJson += ",";
						imagesJson += ",";
					}

					const String name = Stringizer() + ", \"name\":\"" + config.name + "_" + textureTypeNames[t.type] + "\"";

					texturesJson += (Stringizer()
						+ "{\"source\":" + index + name + "}").value.c_str();

					if (t.buff)
					{
						imagesJson += (Stringizer()
							+ "{\"bufferView\":" + t.bufferView
							+ ",\"mimeType\":\"image/png\""
							+ name + "}").value.c_str();
					}
					else
					{
						imagesJson += (Stringizer()
							+ "{\"uri\":\"" + t.tex->filename + "\""
							+ name + "}").value.c_str();
					}

					switch (t.type)
					{
					case 1: // albedo
					{
						if (!materialPbrJson.empty())
							materialPbrJson += ",";
						materialPbrJson += (Stringizer()
							+ "\"baseColorTexture\":{\"index\":" + index + "}").value.c_str();

					} break;
					case 2: // roughness metallic
					{
						if (!materialPbrJson.empty())
							materialPbrJson += ",";
						materialPbrJson += (Stringizer()
							+ "\"metallicRoughnessTexture\":{\"index\":" + index + "}").value.c_str();
					} break;
					case 3: // normal
					{
						if (!materialJson.empty())
							materialJson += ",";
						materialJson += (Stringizer()
							+ "\"normalTexture\":{\"index\":" + index + "}").value.c_str();
					} break;
					}

					index++;
				}

				if (!materialJson.empty())
					materialJson += ",";
				materialJson += "\"pbrMetallicRoughness\":{" + materialPbrJson + "}, \"name\":\"" + config.name.c_str() + "\"";

				std::string tiJson;
				if (!textures.empty())
					tiJson = std::string() + ", \"textures\":[" + texturesJson + "],\"images\":[" + imagesJson + "]";

				json = std::string() +  "{\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[{\"mesh\":0,\"name\":\""
					+ config.name.c_str() + "\"}],\"meshes\":[{\"primitives\":[{\"attributes\":{"
					+ attributesJson + "},\"indices\":" + (Stringizer() + indicesIndex).value.c_str() + ",\"material\":0}],\"name\":\""
					+ config.name.c_str() + "\"}],\"materials\":[{" + materialJson + "}]" + tiJson + ",\"buffers\":[{\"byteLength\":"
					+ (Stringizer() + buffer.size()).value.c_str() + "}],\"bufferViews\":[" + viewsJson + "],\"accessors\":["
					+ accessorsJson + "],\"asset\":{\"version\":\"2.0\"}}";

				while ((json.length() % 4) != 0)
					json += ' ';
			}

			// encode into glb

			MemoryBuffer buffer2;
			buffer2.reserve(json.length() + buffer.size() + 100);
			Serializer glb(buffer2);

			glb << uint32(0x46546C67); // magic
			glb << uint32(2); // version
			Serializer lengthSer = glb.reserve(sizeof(uint32)); // length

			glb << numeric_cast<uint32>(json.length());
			glb << uint32(0x4E4F534A); // json
			glb.write(json);
			while ((buffer2.size() % 4) != 0)
				glb << ' ';

			glb << numeric_cast<uint32>(buffer.size());
			glb << uint32(0x004E4942); // bin
			glb.write(buffer);
			while ((buffer2.size() % 4) != 0)
				glb << '\0';

			lengthSer << numeric_cast<uint32>(buffer2.size());

			return std::move(buffer2);
		}
	}

	Holder<PointerRange<char>> meshExportBuffer(const MeshExportGltfConfig &config)
	{
		return exportImpl("", config);
	}

	void meshExportFiles(const String &filename, const MeshExportGltfConfig &config)
	{
		if (filename.empty())
			CAGE_THROW_ERROR(Exception, "file name cannot be empty");
		Holder<PointerRange<char>> buff = exportImpl(filename, config);
		Holder<File> f = writeFile(filename);
		f->write(buff);
		f->close();
	}
}
