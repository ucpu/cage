#include "processor.h"

Holder<const AssetProcessor> processor;

int main(int argc, const char *args[])
{
	try
	{
		Holder<AssetProcessor> ap = newAssetProcessor();
		processor = ap.share();

		if (argc == 3 && String(args[1]) == "analyze")
		{
			AssetProcessor::initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/analyzeLog/path", "analyze-log"), pathReplaceInvalidCharacters(args[2]) + ".log"));
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "analyzing input: " + args[2]);
			ap->inputDirectory = pathExtractDirectory(args[2]);
			ap->inputName = pathExtractFilename(args[2]);
			ap->derivedProperties();
			return processAnalyze();
		}

		if (argc != 2)
			CAGE_THROW_ERROR(Exception, "missing asset type parameter");

		ap->loadProperties();
		ap->initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/processLog/path", "process-log"), pathReplaceInvalidCharacters(ap->inputName) + ".log"));
		ap->logProperties();

		Delegate<void()> func;
		String component = String(args[1]);
		if (component == "texture")
			func.bind<processTexture>();
		else if (component == "shader")
			func.bind<processShader>();
		else if (component == "pack")
			func.bind<processPack>();
		else if (component == "object")
			func.bind<processObject>();
		else if (component == "animation")
			func.bind<processAnimation>();
		else if (component == "model")
			func.bind<processModel>();
		else if (component == "skeleton")
			func.bind<processSkeleton>();
		else if (component == "font")
			func.bind<processFont>();
		else if (component == "texts")
			func.bind<processTexts>();
		else if (component == "sound")
			func.bind<processSound>();
		else if (component == "collider")
			func.bind<processCollider>();
		else if (component == "raw")
			func.bind<processRaw>();
		else
			CAGE_THROW_ERROR(Exception, "invalid asset type parameter");

		ap->writeLine("cage-begin");
		func();
		ap->writeLine("cage-end");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	AssetProcessor::writeLine("cage-error");
	return 1;
}
