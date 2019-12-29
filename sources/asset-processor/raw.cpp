#include <utility>

#include "processor.h"

#include <cage-core/memoryBuffer.h>

void processRaw()
{
	writeLine(string("use=") + inputFile);

	MemoryBuffer data;
	{ // load file
		Holder<File> f = newFile(inputFile, FileMode(true, false));
		data.allocate(numeric_cast<uintPtr>(f->size()));
		f->read(data.data(), data.size());
	}

	AssetHeader h = initializeAssetHeaderStruct();
	h.originalSize = numeric_cast<uint32>(data.size());

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "original data size: " + data.size() + " bytes");
	if (data.size() >= properties("compressThreshold").toUint32())
	{
		MemoryBuffer data2 = detail::compress(data);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compressed data size: " + data2.size() + " bytes");
		if (data2.size() < data.size())
		{
			std::swap(data, data2);
			CAGE_LOG(SeverityEnum::Info, logComponentName, "using the compressed data");
			h.compressedSize = numeric_cast<uint32>(data2.size());
		}
		else
			CAGE_LOG(SeverityEnum::Info, logComponentName, "using the data without compression");
	}
	else
		CAGE_LOG(SeverityEnum::Info, logComponentName, "data are under compression threshold");

	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(&h, sizeof(h));
	f->write(data.data(), data.size());
	f->close();
}
