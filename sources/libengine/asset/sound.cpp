#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/audio.h>

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

			Holder<Audio> poly = newAudio();
			poly->importBuffer(des.advance(des.available()));
			CAGE_ASSERT(des.available() == 0);
			CAGE_ASSERT(snd.channels == poly->channels());
			CAGE_ASSERT(snd.frames == poly->frames());
			CAGE_ASSERT(snd.sampleRate == poly->sampleRate());
			audioConvertFormat(+poly, AudioFormatEnum::Float);

			Serializer ser(context->originalData);
			ser << snd;
			ser.write(bufferCast<const char, const float>(poly->rawViewFloat()));
			CAGE_ASSERT(ser.available() == 0);
		}

		void processLoad(AssetContext *context)
		{
			Deserializer des(context->originalData.data() ? context->originalData : context->compressedData);
			SoundSourceHeader snd;
			des >> snd;
#ifdef CAGE_ARCHITECTURE_32
			if (snd.frames > (uint32)m)
				CAGE_THROW_ERROR(Exception, "32 bit build of cage cannot handle this large sound file")
#endif // CAGE_ARCHITECTURE_32
			Holder<Audio> poly = newAudio();
			switch (snd.soundType)
			{
			case SoundTypeEnum::RawRaw:
			case SoundTypeEnum::CompressedRaw:
				poly->importRaw(des.advance(des.available()), numeric_cast<uintPtr>(snd.frames), snd.channels, snd.sampleRate, AudioFormatEnum::Float);
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
			Holder<Sound> source = newSound();
			source->referenceDistance = snd.referenceDistance;
			source->rolloffFactor = snd.rolloffFactor;
			source->gain = snd.gain;
			source->loopBeforeStart = any(snd.flags & SoundFlags::LoopBeforeStart);
			source->loopAfterEnd = any(snd.flags & SoundFlags::LoopAfterEnd);
			source->initialize(templates::move(poly));
			context->assetHolder = templates::move(source).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSound(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.decompress.bind<&processDecompress>();
		s.load.bind<&processLoad>();
		return s;
	}
}
