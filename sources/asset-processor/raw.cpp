#include <cage-core/memoryCompression.h>

#include "processor.h"

#include <utility>

void processRaw()
{
	writeLine(string("use=") + inputFile);

	Holder<PointerRange<char>> data;
	{ // load file
		Holder<File> f = readFile(inputFile);
		data = f->readAll();
	}

	AssetHeader h = initializeAssetHeader();
	h.originalSize = numeric_cast<uint32>(data.size());

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "original data size: " + data.size() + " bytes");
	if (data.size() >= toUint32(properties("compressThreshold")))
	{
		Holder<PointerRange<char>> data2 = compress(data);
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
	f->write(bufferView(h));
	f->write(data);
	f->close();
}
