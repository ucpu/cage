#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>

#include <cage-engine/sound.h>
#include <cage-engine/assetStructs.h>

#include "../sound/vorbisDecoder.h"

namespace cage
{
	namespace
	{
		void processDecompress(const AssetContext *context)
		{
			CAGE_ASSERT(context->compressedData().data());
			Deserializer des(context->compressedData());
			SoundSourceHeader snd;
			des >> snd;
			if (snd.soundType != SoundTypeEnum::CompressedRaw)
				return;

			soundPrivat::VorbisData vds;
			uintPtr size = des.available();
			vds.init(des.advance(size).data(), size);
			uint32 ch = 0, f = 0, r = 0;
			vds.decode(ch, f, r, (float*)context->originalData().data());
			CAGE_ASSERT(snd.channels == ch);
			CAGE_ASSERT(snd.frames == f);
			CAGE_ASSERT(snd.sampleRate == r);
		}

		void processLoad(SoundContext *gm, const AssetContext *context)
		{
			Holder<SoundSource> source = newSoundSource(gm);
			source->setDebugName(context->textName);

			Deserializer des(context->compressedData().data() ? context->compressedData() : context->originalData());
			SoundSourceHeader snd;
			des >> snd;
			source->setDataRepeat(any(snd.flags & SoundFlags::LoopBeforeStart), any(snd.flags & SoundFlags::LoopAfterEnd));

			switch (snd.soundType)
			{
			case SoundTypeEnum::RawRaw:
			{
				Deserializer ori(context->originalData());
				ori >> snd;
				source->setDataRaw(snd.channels, snd.frames, snd.sampleRate, bufferCast<const float>(ori.advance(ori.available())));
			} break;
			case SoundTypeEnum::CompressedRaw:
				source->setDataRaw(snd.channels, snd.frames, snd.sampleRate, bufferCast<const float, char>(context->originalData()));
				break;
			case SoundTypeEnum::CompressedCompressed:
			{
				uintPtr size = des.available();
				source->setDataVorbis(des.advance(size));
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid sound type");
			}

			context->assetHolder = templates::move(source).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSoundSource(uint32 threadIndex, SoundContext *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.decompress.bind<&processDecompress>();
		s.load.bind<SoundContext*, &processLoad>(memoryContext);
		return s;
	}
}
