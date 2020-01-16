#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/assetStructs.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>
#include <cage-engine/assetStructs.h>
#include "../sound/vorbisDecoder.h"

namespace cage
{
	namespace
	{
		void processDecompress(const AssetContext *context, void *schemePointer)
		{
			CAGE_ASSERT(context->compressedData().data());
			Deserializer des(context->compressedData());
			SoundSourceHeader snd;
			des >> snd;
			if (snd.soundType != SoundTypeEnum::CompressedRaw)
				return;

			soundPrivat::VorbisData vds;
			uintPtr size = des.available();
			char *data = (char*)des.advance(size);
			vds.init(data, size);
			uint32 ch = 0, f = 0, r = 0;
			vds.decode(ch, f, r, (float*)context->originalData().data());
			CAGE_ASSERT(snd.channels == ch, snd.channels, ch);
			CAGE_ASSERT(snd.frames == f, snd.frames, f);
			CAGE_ASSERT(snd.sampleRate == r, snd.sampleRate, r);
		}

		void processLoad(const AssetContext *context, void *schemePointer)
		{
			SoundContext *gm = (SoundContext*)schemePointer;

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
				source->setDataRaw(snd.channels, snd.frames, snd.sampleRate, (float*)ori.advance(ori.available()));
			} break;
			case SoundTypeEnum::CompressedRaw:
				source->setDataRaw(snd.channels, snd.frames, snd.sampleRate, (float*)context->originalData().data());
				break;
			case SoundTypeEnum::CompressedCompressed:
			{
				uintPtr size = des.available();
				source->setDataVorbis(size, des.advance(size));
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
		s.schemePointer = memoryContext;
		s.decompress.bind<&processDecompress>();
		s.load.bind<&processLoad>();
		return s;
	}
}
