#include "main.h"

#include <cage-core/files.h>
#include <cage-core/logger.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

void testLogger()
{
	CAGE_TESTCASE("logger");

	{
		CAGE_TESTCASE("logging into file interface into memory buffer");
		MemoryBuffer buff;
		Holder<File> f = newFileBuffer(Holder<MemoryBuffer>(&buff, nullptr));
		{
			Holder<LoggerOutputFile> output = newLoggerOutputFile(f.share());
			Holder<Logger> logger = newLogger();
			logger->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(+output);
			CAGE_LOG(SeverityEnum::Info, "loggerTest", "Alea iacta est");
		}
		CAGE_TEST(buff.size() > 0);
		f->seek(0);
		CAGE_TEST(isPattern(f->readLine(), "", "Alea iacta est", ""));
	}
}
