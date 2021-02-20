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
			CAGE_ASSERT(des.available() == 0);
			CAGE_ASSERT(snd.channels == poly->channels());
			CAGE_ASSERT(snd.frames == poly->frames());
			CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
			polytoneConvertFormat(+poly, PolytoneFormatEnum::Float);

			Serializer ser(context->originalData);
			ser << snd;
			ser.write(bufferCast<const char, const float>(poly->rawViewFloat()));
			CAGE_ASSERT(ser.available() == 0);
		}

		void processLoad(AssetContext *context)
		{
			Holder<SoundSource> source = newSoundSource();
			source->setDebugName(context->textName);

			Deserializer des(context->originalData.data() ? context->originalData : context->compressedData);
			SoundSourceHeader snd;
			des >> snd;
			source->setDataRepeat(any(snd.flags & SoundFlags::LoopBeforeStart), any(snd.flags & SoundFlags::LoopAfterEnd));

			Holder<Polytone> poly = newPolytone();
			switch (snd.soundType)
			{
			case SoundTypeEnum::RawRaw:
			case SoundTypeEnum::CompressedRaw:
				poly->importRaw(des.advance(des.available()), snd.frames, snd.channels, snd.sampleRate, PolytoneFormatEnum::Float);
				break;
			case SoundTypeEnum::CompressedCompressed:
				poly->importBuffer(des.advance(des.available()));
				break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid sound type");
			}

			CAGE_ASSERT(des.available() == 0);
			CAGE_ASSERT(snd.channels == poly->channels());
			CAGE_ASSERT(snd.frames == poly->frames());
			CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
			source->setData(templates::move(poly));
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
