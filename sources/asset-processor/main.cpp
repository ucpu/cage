#include <exception>
#include <map>

#include "processor.h"
#include <cage-core/logger.h>
#include <cage-core/hashString.h>

// passed names
string inputDirectory; // c:/asset
string inputName; // path/file?specifier;identifier
string outputDirectory; // c:/data
string outputName; // 123456789
string assetPath;
string schemePath;
uint32 schemeIndex;

// derived names
string inputFileName; // c:/asset/path/file
string outputFileName; // c:/data/123456789
string inputFile; // path/file
string inputSpec; // specifier
string inputIdentifier; // identifier

const char *logComponentName;

assetHeader initializeAssetHeaderStruct()
{
	return initializeAssetHeader(inputName, numeric_cast<uint16>(schemeIndex));
}

namespace
{
	std::map<string, string> props;

	string readLine()
	{
		char buf[string::MaxLength];
		if (fgets(buf, string::MaxLength, stdin) == nullptr)
			CAGE_THROW_ERROR(systemError, "fgets", ferror(stdin));
		return string(buf, numeric_cast<uint32>(detail::strlen(buf) - 1));
	}

	void derivedProperties()
	{
		inputFile = inputName;
		if (inputFile.find(';') != m)
		{
			inputIdentifier = inputFile.split(";");
			std::swap(inputIdentifier, inputFile);
		}
		if (inputFile.find('?') != m)
		{
			inputSpec = inputFile.split("?");
			std::swap(inputSpec, inputFile);
		}

		inputFileName = pathJoin(inputDirectory, inputFile);
		outputFileName = pathJoin(outputDirectory, outputName);
	}

	void loadProperties()
	{
		inputDirectory = readLine();
		inputName = readLine();
		outputDirectory = readLine();
		outputName = readLine();
		assetPath = readLine();
		schemePath = readLine();
		schemeIndex = readLine().toUint32();

		derivedProperties();

		while (true)
		{
			string value = readLine();
			if (value == "cage-end")
				break;
			if (value.find('=') == m)
			{
				CAGE_LOG(severityEnum::Note, "exception", string("line: ") + value);
				CAGE_THROW_ERROR(exception, "missing '=' in property line");
			}
			string name = value.split("=");
			props[name] = value;
		}
	}

	void initializeSecondaryLog(const string &path)
	{
		static holder<loggerOutputFile> *secondaryLogFile = new holder<loggerOutputFile>(); // intentional leak
		static holder<logger> *secondaryLog = new holder<logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<loggerOutputFile, &loggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}
}

void writeLine(const string &other)
{
	CAGE_LOG(severityEnum::Info, "asset-processor", string() + "writing: '" + other + "'");
	{
		string b = other;
		if (b.split("=").trim() == "ref")
			CAGE_LOG(severityEnum::Note, "asset-processor", string() + "reference hash: '" + (uint32)hashString(b.trim().c_str()) + "'");
	}
	if (fprintf(stdout, "%s\n", other.c_str()) < 0)
		CAGE_THROW_ERROR(systemError, "fprintf", ferror(stdout));
}

string properties(const string &name)
{
	auto it = props.find(name);
	if (it != props.end())
		return it->second;
	else
	{
		CAGE_LOG(severityEnum::Note, "exception", string() + "property name: '" + name + "'");
		CAGE_THROW_ERROR(exception, "property not found");
	}
}

int main(int argc, const char *args[])
{
	try
	{
		if (argc == 3 && string(args[1]) == "analyze")
		{
			logComponentName = "analyze";
			inputDirectory = pathExtractPath(args[2]);
			inputName = pathExtractFilename(args[2]);
			derivedProperties();
			initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor.analyze-log.path", "analyze-log"), pathReplaceInvalidCharacters(inputName) + ".log"));
			return processAnalyze();
		}

		if (argc != 2)
			CAGE_THROW_ERROR(exception, "missing asset type parameter");

		loadProperties();
		initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor.process-log.path", "process-log"), pathReplaceInvalidCharacters(inputName) + ".log"));

#define GCHL_GENERATE(N) CAGE_LOG(severityEnum::Info, "asset-processor", string() + "input " CAGE_STRINGIZE(N) ": '" + N + "'");
		CAGE_EVAL_MEDIUM(CAGE_EXPAND_ARGS(GCHL_GENERATE, inputDirectory, inputName, outputDirectory, outputName, assetPath, schemePath, schemeIndex, inputFileName, outputFileName, inputFile, inputSpec, inputIdentifier));
#undef GCHL_GENERATE

		for (const auto &it : props)
			CAGE_LOG(severityEnum::Info, "asset-processor", string() + "property '" + it.first + "': '" + it.second + "'");

		delegate<void()> func;
		string component = string(args[1]);
		if (component == "texture")
			func.bind<&processTexture>();
		else if (component == "shader")
			func.bind<&processShader>();
		else if (component == "pack")
			func.bind<&processPack>();
		else if (component == "object")
			func.bind<&processObject>();
		else if (component == "animation")
			func.bind<&processAnimation>();
		else if (component == "mesh")
			func.bind<&processMesh>();
		else if (component == "skeleton")
			func.bind<&processSkeleton>();
		else if (component == "font")
			func.bind<&processFont>();
		else if (component == "textpack")
			func.bind<&processTextpack>();
		else if (component == "sound")
			func.bind<&processSound>();
		else if (component == "collider")
			func.bind<&processCollider>();
		else if (component == "raw")
			func.bind<&processRaw>();
		else
			CAGE_THROW_ERROR(exception, "invalid asset type parameter");

		logComponentName = args[1];
		writeLine("cage-begin");
		func();
		writeLine("cage-end");
		return 0;
	}
	catch (const cage::exception &)
	{
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Error, "exception", string() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown exception");
	}
	writeLine("cage-error");
	return 1;
}
