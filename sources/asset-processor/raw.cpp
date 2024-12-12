#include "processor.h"

void processRaw()
{
	writeLine(String("use=") + inputFile);

	Holder<PointerRange<char>> data;
	{ // load file
		Holder<File> f = readFile(inputFile);
		data = f->readAll();
	}

	AssetHeader h = initializeAssetHeader();
	h.originalSize = numeric_cast<uint32>(data.size());

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "original data size: " + data.size() + " bytes");
	if (data.size() >= toUint32(properties("compressThreshold")))
	{
		Holder<PointerRange<char>> data2 = memoryCompress(data);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "compressed data size: " + data2.size() + " bytes");
		if (data2.size() < data.size())
		{
			std::swap(data, data2);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using the compressed data");
			h.compressedSize = numeric_cast<uint32>(data2.size());
		}
		else
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using the data without compression");
	}
	else
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "data are under compression threshold");

	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(data);
	f->close();
}
