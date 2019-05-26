#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include "../sound/vorbisDecoder.h"
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>
#include <cage-client/assetStructs.h>

namespace cage
{
	namespace
	{
		void processDecompress(const assetContextStruct *context, void *schemePointer)
		{
			soundHeaderStruct *snd = (soundHeaderStruct*)context->compressedData;
			switch (snd->soundType)
			{
			case soundTypeEnum::RawRaw:
			case soundTypeEnum::CompressedCompressed:
				return; // do nothing
			case soundTypeEnum::CompressedRaw:
				break; // decode now
			default:
				CAGE_THROW_CRITICAL(exception, "invalid sound type");
			}
			soundPrivat::vorbisDataStruct vds;
			vds.init((char*)context->compressedData + sizeof(soundHeaderStruct), numeric_cast<uintPtr>(context->compressedSize - sizeof(soundHeaderStruct)));
			uint32 ch = 0, f = 0, r = 0;
			vds.decode(ch, f, r, (float*)context->originalData);
			CAGE_ASSERT_RUNTIME(snd->channels == ch, snd->channels, ch);
			CAGE_ASSERT_RUNTIME(snd->frames == f, snd->frames, f);
			CAGE_ASSERT_RUNTIME(snd->sampleRate == r, snd->sampleRate, r);
		}

		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			soundContextClass *gm = (soundContextClass*)schemePointer;

			sourceClass *source = nullptr;
			if (context->assetHolder)
			{
				source = static_cast<sourceClass*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSource(gm).cast<void>();
				source = static_cast<sourceClass*>(context->assetHolder.get());
				source->setDebugName(context->textName);
			}
			context->returnData = source;

			soundHeaderStruct *snd = (soundHeaderStruct*)context->compressedData;

			source->setDataRepeat((snd->flags & soundFlags::RepeatBeforeStart) == soundFlags::RepeatBeforeStart, (snd->flags & soundFlags::RepeatAfterEnd) == soundFlags::RepeatAfterEnd);

			switch (snd->soundType)
			{
			case soundTypeEnum::RawRaw:
				source->setDataRaw(snd->channels, snd->frames, snd->sampleRate, (float*)((char*)context->originalData + sizeof(soundHeaderStruct)));
				break;
			case soundTypeEnum::CompressedRaw:
				source->setDataRaw(snd->channels, snd->frames, snd->sampleRate, (float*)context->originalData);
				break;
			case soundTypeEnum::CompressedCompressed:
				source->setDataVorbis(numeric_cast<uintPtr>(context->compressedSize - sizeof(soundHeaderStruct)), ((char*)context->compressedData + sizeof(soundHeaderStruct)));
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid sound type");
			}
		}
	}

	assetSchemeStruct genAssetSchemeSound(uint32 threadIndex, soundContextClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.decompress.bind<&processDecompress>();
		s.load.bind<&processLoad>();
		return s;
	}
}
