#include <cage-core/string.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>

#include <algorithm>

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
			textures.name += Stringizer() + "_ch" + channel;
		}

		void splitChannels(PointerRangeHolder<MeshImportTexture> &textures, const MeshImportPart &part)
		{
			std::vector<MeshImportTexture> add;

			for (MeshImportTexture &it : textures)
			{
				switch (it.type)
				{
				case MeshImportTextureType::AmbientOcclusion:
				{
					switch (it.images.parts[0].image->channels())
					{
					case 1: break;
					case 3: pickChannel(it, 0); break;
					default: CAGE_THROW_ERROR(Exception, "unexpected channels count for ambient occlusion texture");
					}
				} break;
				case MeshImportTextureType::Roughness:
				{
					switch (it.images.parts[0].image->channels())
					{
					case 1: break;
					case 3: pickChannel(it, 1); break;
					case 4: pickChannel(it, 1); break;
					default: CAGE_THROW_ERROR(Exception, "unexpected channels count for roughness texture");
					}
				} break;
				case MeshImportTextureType::Metallic:
				{
					switch (it.images.parts[0].image->channels())
					{
					case 1: break;
					case 3: pickChannel(it, 2); break;
					case 4: pickChannel(it, 2); break;
					default: CAGE_THROW_ERROR(Exception, "unexpected channels count for metallic texture");
					}
				} break;
				case MeshImportTextureType::Emission:
				{
					switch (it.images.parts[0].image->channels())
					{
					case 1: break;
					default: CAGE_THROW_ERROR(Exception, "unexpected channels count for emission texture");
					}
				} break;
				}
			}

			for (auto &it : add)
				textures.push_back(std::move(it));
		}

		void composeAlbedoOpacity(PointerRangeHolder<MeshImportTexture> &textures)
		{
			Holder<Image> channels[4];
			uint32 top = 0;
			String basename;
			String opacityname;

			for (auto &it : textures)
			{
				switch (it.type)
				{
				case MeshImportTextureType::Albedo:
				{
					if (it.images.parts[0].image->channels() == 4)
						return;
					if (!channels[0])
					{
						auto ch = imageChannelsSplit(+it.images.parts[0].image);
						channels[0] = std::move(ch[0]);
						channels[1] = std::move(ch[1]);
						channels[2] = std::move(ch[2]);
						String n = it.name;
						basename = split(n, "?");
					}
					top = 3;
				} break;
				case MeshImportTextureType::Opacity:
				{
					if (!channels[3])
					{
						channels[3] = it.images.parts[0].image.share();
						String n = it.name;
						opacityname = split(n, "?");
					}
					top = 4;
				} break;
				}
			}

			if (top != 4)
				return;

			std::erase_if(textures, [](const MeshImportTexture &it) {
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
			res.name = Stringizer() + basename + "?opacity_" + opacityname;
			res.type = MeshImportTextureType::Albedo;
			textures.push_back(std::move(res));
		}

		void composeSpecial(PointerRangeHolder<MeshImportTexture> &textures)
		{
			Holder<Image> channels[4];
			uint32 top = 0;
			String base;
			String names;

			const auto &process = [&](const MeshImportTexture &in, uint32 index) {
				if (channels[index])
					return;
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
				}
			}

			if (top == 0)
				return;

			std::erase_if(textures, [](const MeshImportTexture &it) {
				switch (it.type)
				{
				case MeshImportTextureType::Roughness:
				case MeshImportTextureType::Metallic:
				case MeshImportTextureType::Emission:
				case MeshImportTextureType::Mask:
					return true;
				default:
					return false;
				}
			});

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
	}

	namespace privat
	{
		void meshImportConvertToCageFormats(MeshImportPart &part)
		{
			std::vector<MeshImportTexture> unloadable;
			PointerRangeHolder<MeshImportTexture> textures;
			textures.reserve(part.textures.size());
			for (auto &it : part.textures)
			{
				if (it.images.parts.empty())
				{
					if (cage::isPattern(it.name, "", "?", "") || cage::isPattern(it.name, "", ";", ""))
					{
						unloadable.push_back(std::move(it));
						continue;
					}
					it.images = imageImportFiles(it.name);
				}
				imageImportConvertRawToImages(it.images);
				textures.push_back(std::move(it));
			}

			splitChannels(textures, part);

			std::sort(textures.begin(), textures.end(), [](const MeshImportTexture &a, const MeshImportTexture &b) {
				return a.type < b.type;
			});

			if (any(part.renderFlags & (MeshRenderFlags::Translucent | MeshRenderFlags::AlphaClip)))
				composeAlbedoOpacity(textures);
			composeSpecial(textures);

			for (auto &it : unloadable)
				textures.push_back(std::move(it));

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

	void meshImportConvertToCageFormats(MeshImportResult &result)
	{
		for (MeshImportPart &p : result.parts)
			privat::meshImportConvertToCageFormats(p);
	}
}
