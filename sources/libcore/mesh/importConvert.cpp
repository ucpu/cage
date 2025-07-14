#include <algorithm>

#include <cage-core/color.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/meshAlgorithms.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
		void pickChannel(MeshImportTexture &textures, uint32 channel)
		{
			for (auto &it : textures.images.parts)
			{
				auto vals = imageChannelsSplit(+it.image);
				if (channel >= vals.size())
					CAGE_THROW_ERROR(Exception, "channel index out of range");
				it.image = std::move(vals[channel]);
			}
			if (find(textures.name, '?') != m)
				textures.name += Stringizer() + "_ch" + channel;
		}

		void limitChannels(MeshImportTexture &textures, uint32 channels)
		{
			for (auto &it : textures.images.parts)
			{
				auto vals = imageChannelsSplit(+it.image);
				if (channels > vals.size())
					CAGE_THROW_ERROR(Exception, "channel index out of range");
				it.image = imageChannelsJoin(PointerRange{ vals.begin(), vals.begin() + channels });
			}
			if (find(textures.name, '?') != m)
				textures.name += Stringizer() + "_lch" + channels;
		}

		void averageChannels3To1(MeshImportTexture &textures)
		{
			for (auto &it : textures.images.parts)
			{
				Holder<Image> img = newImage();
				const Vec2i res = it.image->resolution();
				img->initialize(res, 1, it.image->format());
				for (uint32 y = 0; y < res[1]; y++)
				{
					for (uint32 x = 0; x < res[0]; x++)
					{
						const Vec3 c = it.image->get3(x, y);
						const Real v = (c[0] + c[1] + c[2]) / 3;
						img->set(x, y, v);
					}
				}
				it.image = std::move(img);
			}
			if (find(textures.name, '?') != m)
				textures.name += Stringizer() + "_avg";
		}

		void expandChannels1To3(MeshImportTexture &textures)
		{
			for (auto &it : textures.images.parts)
			{
				const Image *arr[3] = { +it.image, +it.image, +it.image };
				it.image = imageChannelsJoin(arr);
			}
			if (find(textures.name, '?') != m)
				textures.name += Stringizer() + "_exp";
		}

		void normalizeChannels(MeshImportTexture &it)
		{
			if (it.images.parts[0].image->channels() == 0)
				CAGE_THROW_ERROR(Exception, "texture with zero channels");
			switch (it.type)
			{
				case MeshImportTextureType::Albedo:
				{
					if (it.images.parts[0].image->channels() > 4)
						CAGE_THROW_ERROR(Exception, "too many channels for albedo texture");
					break;
				}
				case MeshImportTextureType::Normal:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							it.type = MeshImportTextureType::Bump;
							break;
						case 2:
							break;
						case 3:
							break;
						case 4:
							limitChannels(it, 3);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "too many channels for normal texture");
					}
					break;
				}
				case MeshImportTextureType::Special:
				{
					if (it.images.parts[0].image->channels() > 4)
						CAGE_THROW_ERROR(Exception, "too many channels for special texture");
					break;
				}
				case MeshImportTextureType::Custom:
					break; // no limits
				case MeshImportTextureType::AmbientOcclusion:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 3:
							pickChannel(it, 0);
							break;
						case 4:
							pickChannel(it, 0);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for ambient occlusion texture");
					}
					break;
				}
				case MeshImportTextureType::Anisotropy:
				{
					// unknown
					break;
				}
				case MeshImportTextureType::Bump:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 3:
							pickChannel(it, 1);
							break;
						case 4:
							pickChannel(it, 1);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for bump texture");
					}
					break;
				}
				case MeshImportTextureType::Clearcoat:
				{
					// unknown
					break;
				}
				case MeshImportTextureType::Emission:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 3:
							averageChannels3To1(it);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for emission texture");
					}
					break;
				}
				case MeshImportTextureType::GltfPbr:
				{
					if (it.images.parts[0].image->channels() > 4)
						CAGE_THROW_ERROR(Exception, "too many channels for gltf pbr texture");
					break;
				}
				case MeshImportTextureType::Mask:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for mask texture");
					}
					break;
				}
				case MeshImportTextureType::Metallic:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 3:
							pickChannel(it, 2);
							break;
						case 4:
							pickChannel(it, 2);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for metallic texture");
					}
					break;
				}
				case MeshImportTextureType::Opacity:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 2:
							pickChannel(it, 1);
							break;
						case 4:
							pickChannel(it, 3);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for opacity texture");
					}
					break;
				}
				case MeshImportTextureType::Roughness:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						case 3:
							pickChannel(it, 1);
							break;
						case 4:
							pickChannel(it, 1);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for roughness texture");
					}
					break;
				}
				case MeshImportTextureType::Sheen:
				{
					// unknown
					break;
				}
				case MeshImportTextureType::Shininess:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							break;
						default:
							CAGE_THROW_ERROR(Exception, "unexpected channels count for shininess texture");
					}
					break;
				}
				case MeshImportTextureType::Specular:
				{
					switch (it.images.parts[0].image->channels())
					{
						case 1:
							expandChannels1To3(it);
							break;
						case 3:
							break;
						case 4:
							limitChannels(it, 3);
							break;
						default:
							CAGE_THROW_ERROR(Exception, "too many channels for specular texture");
					}
					break;
				}
				case MeshImportTextureType::Transmission:
				{
					// unknown
					break;
				}
			}
		}

		uint32 count(PointerRange<const MeshImportTexture> textures, MeshImportTextureType type)
		{
			uint32 res = 0;
			for (const auto &it : textures)
				res += it.type == type;
			return res;
		}

		void composeAlbedoOpacity(PointerRangeHolder<MeshImportTexture> &textures)
		{
			const auto &find = [&](MeshImportTextureType type) -> const MeshImportTexture *
			{
				for (const auto &it : textures)
					if (it.type == type)
						return &it;
				return nullptr;
			};

			const MeshImportTexture *a = find(MeshImportTextureType::Albedo);
			const MeshImportTexture *o = find(MeshImportTextureType::Opacity);

			if (!a || !o)
				return;

			if (a->images.parts[0].image->channels() == 4)
			{
				std::erase_if(textures, [](const MeshImportTexture &it) { return it.type == MeshImportTextureType::Opacity; });
				return;
			}

			CAGE_ASSERT(a->images.parts[0].image->channels() == 3);
			CAGE_ASSERT(o->images.parts[0].image->channels() == 1);

			Holder<Image> channels[4];
			{
				auto ch = imageChannelsSplit(+a->images.parts[0].image);
				channels[0] = std::move(ch[0]);
				channels[1] = std::move(ch[1]);
				channels[2] = std::move(ch[2]);
			}
			channels[3] = std::move(o->images.parts[0].image);

			String n1 = a->name, n2 = o->name;
			const String name = Stringizer() + split(n1, "?") + "?opacity_" + split(n2, "?");

			std::erase_if(textures,
				[](const MeshImportTexture &it)
				{
					switch (it.type)
					{
						case MeshImportTextureType::Albedo:
						case MeshImportTextureType::Opacity:
							return true;
						default:
							return false;
					}
				});

			ImageImportPart part;
			part.image = imageChannelsJoin(channels);
			PointerRangeHolder<ImageImportPart> parts;
			parts.push_back(std::move(part));
			MeshImportTexture res;
			res.images.parts = std::move(parts);
			res.name = name;
			res.type = MeshImportTextureType::Albedo;
			textures.push_back(std::move(res));
		}

		void convertGltfToSpecial(PointerRangeHolder<MeshImportTexture> &textures)
		{
			if (count(textures, MeshImportTextureType::Special) > 0)
				return;

			const auto &find = [&](MeshImportTextureType type) -> MeshImportTexture *
			{
				for (auto &it : textures)
					if (it.type == type)
						return &it;
				return nullptr;
			};

			MeshImportTexture *gltf = find(MeshImportTextureType::GltfPbr);
			if (!gltf)
				return;

			Holder<Image> channels[4];
			{
				auto ch = imageChannelsSplit(+gltf->images.parts[0].image);
				if (ch.size() > 1)
					channels[0] = std::move(ch[1]);
				if (ch.size() > 2)
					channels[1] = std::move(ch[2]);
				if (ch.size() > 3)
					channels[3] = std::move(ch[3]);
			}

			if (MeshImportTexture *em = find(MeshImportTextureType::Emission))
				channels[2] = std::move(em->images.parts[0].image);

			if (!channels[3])
			{
				if (MeshImportTexture *em = find(MeshImportTextureType::Mask))
					channels[3] = std::move(em->images.parts[0].image);
			}

			{
				ImageImportPart p;
				p.image = imageChannelsJoin(channels);
				PointerRangeHolder<ImageImportPart> ps;
				ps.push_back(std::move(p));
				gltf->images.parts = std::move(ps);
				gltf->type = MeshImportTextureType::Special;
			}
		}

		void convertSpecularToRoughnessMetallic(PointerRangeHolder<MeshImportTexture> &textures)
		{
			if (count(textures, MeshImportTextureType::Special) > 0)
				return;

			const auto &find = [&](MeshImportTextureType type) -> MeshImportTexture *
			{
				for (auto &it : textures)
					if (it.type == type)
						return &it;
				return nullptr;
			};

			MeshImportTexture *spec = find(MeshImportTextureType::Specular);
			if (!spec)
				return;
			const Image *src = +spec->images.parts[0].image;
			const Vec2i res = src->resolution();

			Holder<Image> ri = newImage();
			ri->initialize(res, 1, src->format());
			Holder<Image> mi = newImage();
			mi->initialize(res, 1, src->format());

			for (uint32 y = 0; y < res[1]; y++)
			{
				for (uint32 x = 0; x < res[0]; x++)
				{
					const Vec2 rm = colorSpecularToRoughnessMetallic(src->get3(x, y));
					ri->set(x, y, rm[0]);
					mi->set(x, y, rm[1]);
				}
			}

			{
				ImageImportPart p;
				p.image = std::move(ri);
				PointerRangeHolder<ImageImportPart> ps;
				ps.push_back(std::move(p));
				MeshImportTexture res;
				res.images.parts = std::move(ps);
				res.type = MeshImportTextureType::Roughness;
				textures.push_back(std::move(res));
			}

			{
				ImageImportPart p;
				p.image = std::move(mi);
				PointerRangeHolder<ImageImportPart> ps;
				ps.push_back(std::move(p));
				MeshImportTexture res;
				res.images.parts = std::move(ps);
				res.type = MeshImportTextureType::Metallic;
				textures.push_back(std::move(res));
			}
		}

		void composeSpecial(PointerRangeHolder<MeshImportTexture> &textures)
		{
			if (count(textures, MeshImportTextureType::Special) > 0)
				return;

			Holder<Image> channels[4];
			uint32 top = 0;
			String base;
			String names;

			const auto &process = [&](const MeshImportTexture &in, uint32 index)
			{
				if (channels[index])
					return;
				CAGE_ASSERT(in.images.parts[0].image->channels() == 1);
				channels[index] = in.images.parts[0].image.share();
				String n = in.name;
				String b = split(n, "?");
				if (base.empty())
					base = b;
				if (!names.empty())
					names += "_";
				names += n;
				top = cage::max(top, index + 1);
			};

			for (auto &it : textures)
			{
				switch (it.type)
				{
					case MeshImportTextureType::Roughness:
						process(it, 0);
						break;
					case MeshImportTextureType::Metallic:
						process(it, 1);
						break;
					case MeshImportTextureType::Emission:
						process(it, 2);
						break;
					case MeshImportTextureType::Mask:
						process(it, 3);
						break;
					default:
						break;
				}
			}

			if (top == 0)
				return;

			ImageImportPart part;
			part.image = imageChannelsJoin({ channels, channels + top });
			PointerRangeHolder<ImageImportPart> parts;
			parts.push_back(std::move(part));
			MeshImportTexture res;
			res.images.parts = std::move(parts);
			res.name = Stringizer() + base + "?special_" + names;
			res.type = MeshImportTextureType::Special;
			textures.push_back(std::move(res));
		}

		void filterTextures(PointerRangeHolder<MeshImportTexture> &textures)
		{
			std::erase_if(textures,
				[](const MeshImportTexture &it)
				{
					switch (it.type)
					{
						case MeshImportTextureType::Albedo:
						case MeshImportTextureType::Special:
						case MeshImportTextureType::Normal:
						case MeshImportTextureType::Bump:
						case MeshImportTextureType::Custom:
							return false;
						default:
							return true;
					}
				});

			// remove duplicate textures (preserves the first one of each type)
			std::stable_sort(textures.begin(), textures.end(), [](const MeshImportTexture &a, const MeshImportTexture &b) { return a.type < b.type; });
			textures.erase(std::unique(textures.begin(), textures.end(), [](const MeshImportTexture &a, const MeshImportTexture &b) { return a.type == b.type; }), textures.end());

			// remove bump map if normal map is present
			if (count(textures, MeshImportTextureType::Normal) > 0)
				std::erase_if(textures, [](const MeshImportTexture &it) { return it.type == MeshImportTextureType::Bump; });
		}
	}

	namespace privat
	{
		void meshImportNormalizeFormats(MeshImportPart &part)
		{
			meshConvertToIndexed(+part.mesh);
			for (auto &it : part.textures)
			{
				if (it.images.parts.empty())
				{
					if (find(it.name, '?') != m || find(it.name, ';') != m)
						continue;
					try
					{
						it.images = imageImportFiles(it.name);
					}
					catch (...)
					{
						CAGE_LOG_THROW(Stringizer() + "error while loading image: " + it.name);
						throw;
					}
				}
				imageImportConvertRawToImages(it.images);
				normalizeChannels(it);
			}
		}

		void meshImportConvertToCageFormats(MeshImportPart &part)
		{
			meshImportNormalizeFormats(part);

			std::vector<MeshImportTexture> inaccessible;
			PointerRangeHolder<MeshImportTexture> textures;
			textures.reserve(part.textures.size());
			for (auto &it : part.textures)
			{
				if (it.images.parts.empty())
					inaccessible.push_back(std::move(it));
				else
					textures.push_back(std::move(it));
			}
			std::stable_sort(textures.begin(), textures.end(), [](const MeshImportTexture &a, const MeshImportTexture &b) { return a.type < b.type; });

			composeAlbedoOpacity(textures);
			convertGltfToSpecial(textures);
			convertSpecularToRoughnessMetallic(textures);
			composeSpecial(textures);

			for (auto &it : inaccessible)
				textures.push_back(std::move(it));
			filterTextures(textures);

			for (auto &it : textures)
			{
				if (find(it.name, '?') != m)
					it.name = Stringizer() + split(it.name, "?") + "?" + part.objectName + "_" + meshImportTextureTypeToString(it.type);
			}

			part.textures = std::move(textures);
		}
	}

	void meshImportLoadExternal(MeshImportResult &result)
	{
		for (MeshImportPart &part : result.parts)
			for (auto &it : part.textures)
				if (it.images.parts.empty())
					it.images = imageImportFiles(it.name);
	}

	void meshImportNormalizeFormats(MeshImportResult &result)
	{
		for (MeshImportPart &p : result.parts)
			privat::meshImportNormalizeFormats(p);
	}

	void meshImportConvertToCageFormats(MeshImportResult &result)
	{
		for (MeshImportPart &p : result.parts)
			privat::meshImportConvertToCageFormats(p);
	}

	MeshImportResult meshImportMerge(PointerRange<const MeshImportResult> inputs)
	{
		if (inputs.empty())
			return {};
		PointerRangeHolder<String> paths;
		std::vector<MeshMergeInput> mis;
		for (const auto &in : inputs)
		{
			if (in.skeleton && inputs.size() > 1)
				CAGE_THROW_ERROR(Exception, "cannot merge mesh parts with skeletons");
			for (const auto &pt : in.parts)
			{
				MeshMergeInput mi;
				mi.mesh = +pt.mesh;
				// todo textures
				// todo material
				mis.push_back(std::move(mi));
			}
			for (const String &p : in.paths)
				paths.push_back(p);
		}
		auto merged = meshMerge(mis);
		PointerRangeHolder<MeshImportPart> parts;
		MeshImportPart prt;
		prt.mesh = std::move(merged.mesh);
		// todo textures
		parts.push_back(std::move(prt));
		MeshImportResult r;
		r.parts = std::move(parts);
		r.paths = std::move(paths);
		if (inputs[0].skeleton)
		{
			r.skeleton = inputs[0].skeleton.share();
			r.animations = inputs[0].animations.share();
		}
		return r;
	}
}
