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

		void splitChannels(MeshImportTexture &it)
		{
			switch (it.type)
			{
			case MeshImportTextureType::AmbientOcclusion:
			{
				switch (it.images.parts[0].image->channels())
				{
				case 1: break;
				case 3: pickChannel(it, 0); break;
				case 4: pickChannel(it, 0); break;
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
			case MeshImportTextureType::Opacity:
			{
				switch (it.images.parts[0].image->channels())
				{
				case 1: break;
				case 2: pickChannel(it, 1); break;
				case 4: pickChannel(it, 3); break;
				default: CAGE_THROW_ERROR(Exception, "unexpected channels count for opacity texture");
				}
			} break;
			}
		}

		void composeAlbedoOpacity(PointerRangeHolder<MeshImportTexture> &textures)
		{
			const auto &find = [&](MeshImportTextureType type) -> const MeshImportTexture * {
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
			res.name = name;
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
		void meshImportNormalizeFormats(MeshImportPart &part)
		{
			for (auto &it : part.textures)
			{
				if (it.images.parts.empty())
				{
					if (cage::isPattern(it.name, "", "?", "") || cage::isPattern(it.name, "", ";", ""))
						continue;
					it.images = imageImportFiles(it.name);
				}
				imageImportConvertRawToImages(it.images);
				splitChannels(it);
			}
		}

		void meshImportConvertToCageFormats(MeshImportPart &part)
		{
			meshImportNormalizeFormats(part);

			std::vector<MeshImportTexture> unloadable;
			PointerRangeHolder<MeshImportTexture> textures;
			textures.reserve(part.textures.size());
			for (auto &it : part.textures)
			{
				if (it.images.parts.empty())
					unloadable.push_back(std::move(it));
				else
					textures.push_back(std::move(it));
			}

			std::stable_sort(textures.begin(), textures.end(), [](const MeshImportTexture &a, const MeshImportTexture &b) {
				return a.type < b.type;
			});

			composeAlbedoOpacity(textures);
			composeSpecial(textures);

			for (auto &it : unloadable)
				textures.push_back(std::move(it));

			std::erase_if(textures, [](const MeshImportTexture &it) {
				switch (it.type)
				{
				case MeshImportTextureType::Albedo:
				case MeshImportTextureType::Special:
				case MeshImportTextureType::Normal:
				case MeshImportTextureType::Bump:
					return false;
				default:
					return true;
				}
			});

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
}
