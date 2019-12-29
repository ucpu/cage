#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include "../sound/vorbisDecoder.h"
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processDecompress(const AssetContext *context, void *schemePointer)
		{
			soundSourceHeader *snd = (soundSourceHeader*)context->compressedData;
			switch (snd->soundType)
			{
			case soundTypeEnum::RawRaw:
			case soundTypeEnum::CompressedCompressed:
				return; // do nothing
			case soundTypeEnum::CompressedRaw:
				break; // decode now
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid sound type");
			}
			soundPrivat::vorbisDataStruct vds;
			vds.init((char*)context->compressedData + sizeof(soundSourceHeader), numeric_cast<uintPtr>(context->compressedSize - sizeof(soundSourceHeader)));
			uint32 ch = 0, f = 0, r = 0;
			vds.decode(ch, f, r, (float*)context->originalData);
			CAGE_ASSERT(snd->channels == ch, snd->channels, ch);
			CAGE_ASSERT(snd->frames == f, snd->frames, f);
			CAGE_ASSERT(snd->sampleRate == r, snd->sampleRate, r);
		}

		void processLoad(const AssetContext *context, void *schemePointer)
		{
			soundContext *gm = (soundContext*)schemePointer;

			soundSource *source = nullptr;
			if (context->assetHolder)
			{
				source = static_cast<soundSource*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSoundSource(gm).cast<void>();
				source = static_cast<soundSource*>(context->assetHolder.get());
				source->setDebugName(context->textName);
			}
			context->returnData = source;

			soundSourceHeader *snd = (soundSourceHeader*)context->compressedData;

			source->setDataRepeat((snd->flags & soundFlags::LoopBeforeStart) == soundFlags::LoopBeforeStart, (snd->flags & soundFlags::LoopAfterEnd) == soundFlags::LoopAfterEnd);

			switch (snd->soundType)
			{
			case soundTypeEnum::RawRaw:
				source->setDataRaw(snd->channels, snd->frames, snd->sampleRate, (float*)((char*)context->originalData + sizeof(soundSourceHeader)));
				break;
			case soundTypeEnum::CompressedRaw:
				source->setDataRaw(snd->channels, snd->frames, snd->sampleRate, (float*)context->originalData);
				break;
			case soundTypeEnum::CompressedCompressed:
				source->setDataVorbis(numeric_cast<uintPtr>(context->compressedSize - sizeof(soundSourceHeader)), ((char*)context->compressedData + sizeof(soundSourceHeader)));
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid sound type");
			}
		}
	}

	AssetScheme genAssetSchemeSoundSource(uint32 threadIndex, soundContext *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.decompress.bind<&processDecompress>();
		s.load.bind<&processLoad>();
		return s;
	}
}
