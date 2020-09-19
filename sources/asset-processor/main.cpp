#include <cage-core/logger.h>
#include <cage-core/hashString.h>

#include "processor.h"

#include <map>
#include <cstdio> // fgets, ferror
#include <cstring> // strlen

// passed names
string inputDirectory; // c:/asset
string inputName; // path/file?specifier;identifier
string outputDirectory; // c:/data
string outputName; // 123456789
uint32 schemeIndex;

// derived names
string inputFileName; // c:/asset/path/file
string outputFileName; // c:/data/123456789
string inputFile; // path/file
string inputSpec; // specifier
string inputIdentifier; // identifier

const char *logComponentName;

AssetHeader initializeAssetHeader()
{
	AssetHeader h = initializeAssetHeader(inputName, numeric_cast<uint16>(schemeIndex));
	string intr = properties("alias");
	if (!intr.empty())
	{
		intr = pathJoin(pathExtractPath(inputName), intr);
		writeLine(stringizer() + "alias = " + intr);
		h.aliasName = HashString(intr);
	}
	return h;
}

namespace
{
	std::map<string, string> props;

	string readLine()
	{
		char buf[string::MaxLength];
		if (std::fgets(buf, string::MaxLength, stdin) == nullptr)
			CAGE_THROW_ERROR(SystemError, "fgets", std::ferror(stdin));
		return trim(string(buf));
	}

	void derivedProperties()
	{
		inputFile = inputName;
		if (find(inputFile, ';') != m)
		{
			inputIdentifier = split(inputFile, ";");
			std::swap(inputIdentifier, inputFile);
		}
		if (find(inputFile, '?') != m)
		{
			inputSpec = split(inputFile, "?");
			std::swap(inputSpec, inputFile);
		}

		inputFileName = pathJoin(inputDirectory, inputFile);
		outputFileName = pathJoin(outputDirectory, outputName);
	}

	void loadProperties()
	{
		while (true)
		{
			string value = readLine();
			if (value == "cage-end")
				break;
			if (find(value, '=') == m)
			{
				CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "line: " + value);
				CAGE_THROW_ERROR(Exception, "missing '=' in property line");
			}
			string name = split(value, "=");
			props[name] = value;
		}

		inputDirectory = properties("inputDirectory");
		inputName = properties("inputName");
		outputDirectory = properties("outputDirectory");
		outputName = properties("outputName");
		schemeIndex = toUint32(properties("schemeIndex"));
		derivedProperties();
	}

	void initializeSecondaryLog(const string &path)
	{
		static Holder<LoggerOutputFile> *secondaryLogFile = new Holder<LoggerOutputFile>(); // intentional leak
		static Holder<Logger> *secondaryLog = new Holder<Logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}
}

void writeLine(const string &other)
{
	CAGE_LOG(SeverityEnum::Info, "asset-processor", stringizer() + "writing: '" + other + "'");
	{
		string b = other;
		if (trim(split(b, "=")) == "ref")
			CAGE_LOG(SeverityEnum::Note, "asset-processor", stringizer() + "reference hash: '" + (uint32)HashString(trim(b).c_str()) + "'");
	}
	if (fprintf(stdout, "%s\n", other.c_str()) < 0)
		CAGE_THROW_ERROR(SystemError, "fprintf", ferror(stdout));
}

string properties(const string &name)
{
	auto it = props.find(name);
	if (it != props.end())
		return it->second;
	else
	{
		CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "property name: '" + name + "'");
		CAGE_THROW_ERROR(Exception, "property not found");
	}
}

int main(int argc, const char *args[])
{
	try
	{
		if (argc == 3 && string(args[1]) == "analyze")
		{
			logComponentName = "analyze";
			initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/analyzeLog/path", "analyze-log"), pathReplaceInvalidCharacters(args[2]) + ".log"));
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "analyzing input '" + args[2] + "'");
			inputDirectory = pathExtractPath(args[2]);
			inputName = pathExtractFilename(args[2]);
			derivedProperties();
			return processAnalyze();
		}

		if (argc != 2)
			CAGE_THROW_ERROR(Exception, "missing asset type parameter");

		loadProperties();
		initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/processLog/path", "process-log"), pathReplaceInvalidCharacters(inputName) + ".log"));

		for (const auto &it : props)
			CAGE_LOG(SeverityEnum::Info, "asset-processor", stringizer() + "property '" + it.first + "': '" + it.second + "'");

#define GCHL_GENERATE(N) CAGE_LOG(SeverityEnum::Info, "asset-processor", stringizer() + "input " #N ": '" + N + "'");
		GCHL_GENERATE(inputFileName);
		GCHL_GENERATE(outputFileName);
		GCHL_GENERATE(inputFile);
		GCHL_GENERATE(inputSpec);
		GCHL_GENERATE(inputIdentifier);
#undef GCHL_GENERATE

		Delegate<void()> func;
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
			CAGE_THROW_ERROR(Exception, "invalid asset type parameter");

		logComponentName = args[1];
		writeLine("cage-begin");
		func();
		writeLine("cage-end");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	writeLine("cage-error");
	return 1;
}
