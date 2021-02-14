#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/polytone.h>

#include <cage-engine/sound.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processDecompress(AssetContext *context)
		{
			CAGE_ASSERT(context->compressedData.data());
			Deserializer des(context->compressedData);
			SoundSourceHeader snd;
			des >> snd;
			if (snd.soundType != SoundTypeEnum::CompressedRaw)
				return;

			Holder<Polytone> poly = newPolytone();
			poly->importBuffer(des.advance(des.available()));
			CAGE_ASSERT(snd.channels == poly->channels());
			CAGE_ASSERT(snd.frames == poly->frames());
			CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
			polytoneConvertFormat(+poly, PolytoneFormatEnum::Float);
			detail::memcpy(context->originalData.data(), poly->rawViewFloat().data(), context->originalData.size());
		}

		void processLoad(AssetContext *context)
		{
			Holder<SoundSource> source = newSoundSource();
			source->setDebugName(context->textName);

			Deserializer des(context->compressedData.data() ? context->compressedData : context->originalData);
			SoundSourceHeader snd;
			des >> snd;
			source->setDataRepeat(any(snd.flags & SoundFlags::LoopBeforeStart), any(snd.flags & SoundFlags::LoopAfterEnd));

			switch (snd.soundType)
			{
			case SoundTypeEnum::RawRaw:
			{
				Deserializer des(context->originalData);
				des >> snd;
				Holder<Polytone> poly = newPolytone();
				poly->importRaw(des.advance(des.available()), snd.frames, snd.channels, snd.sampleRate, PolytoneFormatEnum::Float);
				CAGE_ASSERT(snd.channels == poly->channels());
				CAGE_ASSERT(snd.frames == poly->frames());
				CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
				source->setData(templates::move(poly));
			} break;
			case SoundTypeEnum::CompressedRaw:
			{
				Holder<Polytone> poly = newPolytone();
				poly->importBuffer(context->originalData);
				CAGE_ASSERT(snd.channels == poly->channels());
				CAGE_ASSERT(snd.frames == poly->frames());
				CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
				polytoneConvertFormat(+poly, PolytoneFormatEnum::Float);
				source->setData(templates::move(poly));
			} break;
			case SoundTypeEnum::CompressedCompressed:
			{
				Holder<Polytone> poly = newPolytone();
				poly->importBuffer(des.advance(des.available()));
				CAGE_ASSERT(snd.channels == poly->channels());
				CAGE_ASSERT(snd.frames == poly->frames());
				CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
				source->setData(templates::move(poly));
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid sound type");
			}

			context->assetHolder = templates::move(source).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSoundSource(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.decompress.bind<&processDecompress>();
		s.load.bind<&processLoad>();
		return s;
	}
}
