#include <cstdio> // fgets, ferror
#include <cstring> // strlen
#include <map>

#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/logger.h>

// passed names
String inputDirectory; // c:/asset
String inputName; // path/file?specifier;identifier
String outputDirectory; // c:/data
String outputName; // 123456789
uint32 schemeIndex;

// derived names
String inputFileName; // c:/asset/path/file
String outputFileName; // c:/data/123456789
String inputFile; // path/file
String inputSpec; // specifier
String inputIdentifier; // identifier

AssetHeader initializeAssetHeader()
{
	return AssetHeader(inputName, numeric_cast<uint16>(schemeIndex));
}

namespace
{
	std::map<String, String> props;

	String readLine()
	{
		char buf[String::MaxLength];
		if (std::fgets(buf, String::MaxLength, stdin) == nullptr)
			CAGE_THROW_ERROR(SystemError, "fgets", std::ferror(stdin));
		return trim(String(buf));
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
			String value = readLine();
			if (value == "cage-end")
				break;
			if (find(value, '=') == m)
			{
				CAGE_LOG_THROW(Stringizer() + "line: " + value);
				CAGE_THROW_ERROR(Exception, "missing '=' in property line");
			}
			String name = split(value, "=");
			props[name] = value;
		}

		inputDirectory = properties("inputDirectory");
		inputName = properties("inputName");
		outputDirectory = properties("outputDirectory");
		outputName = properties("outputName");
		schemeIndex = toUint32(properties("schemeIndex"));
		derivedProperties();
	}

	void initializeSecondaryLog(const String &path)
	{
		static Holder<LoggerOutputFile> *secondaryLogFile = new Holder<LoggerOutputFile>(); // intentional leak
		static Holder<Logger> *secondaryLog = new Holder<Logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<&logFormatFileShort>();
	}

	String convertGenericPath(const String &input_, const String &relativeTo_)
	{
		String input = input_;
		if (isPattern(input, inputDirectory, "", ""))
			input = subString(input, inputDirectory.length(), m);
		if (input.empty())
			CAGE_THROW_ERROR(Exception, "input cannot be empty");
		if (pathIsAbs(input))
		{
			if (input[0] != '/')
				CAGE_THROW_ERROR(Exception, "absolute path with protocol");
			while (!input.empty() && input[0] == '/')
				split(input, "/");
		}
		else
		{
			const String relativeTo = relativeTo_.empty() ? pathExtractDirectory(inputFile) : relativeTo_;
			input = pathJoin(relativeTo, input);
		}
		if (input.empty())
			CAGE_THROW_ERROR(Exception, "input cannot be empty");
		return input;
	}

	String separateDetail(String &p)
	{
		const uint32 sep = min(find(p, '?'), find(p, ';'));
		const String detail = subString(p, sep, m);
		p = subString(p, 0, sep);
		return detail;
	}
}

String convertAssetPath(const String &input, const String &relativeTo, bool markAsReferenced)
{
	String p = input;
	const String detail = separateDetail(p);
	p = convertGenericPath(p, relativeTo) + detail;
	if (markAsReferenced)
		writeLine(String("ref=") + p);
	return p;
}

String convertFilePath(const String &input, const String &relativeTo, bool markAsUsed)
{
	String p = input;
	const String detail = separateDetail(p);
	p = convertGenericPath(p, relativeTo);
	if (markAsUsed)
		writeLine(String("use=") + p);
	return p + detail;
}

void writeLine(const String &other)
{
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "writing: '" + other + "'");
	{
		String b = other;
		if (trim(split(b, "=")) == "ref")
			CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "reference hash: '" + (uint32)HashString(trim(b)) + "'");
	}
	if (fprintf(stdout, "%s\n", other.c_str()) < 0)
		CAGE_THROW_ERROR(SystemError, "fprintf", ferror(stdout));
}

String properties(const String &name)
{
	auto it = props.find(name);
	if (it != props.end())
		return it->second;
	else
	{
		CAGE_LOG_THROW(Stringizer() + "property name: '" + name + "'");
		CAGE_THROW_ERROR(Exception, "property not found");
	}
}

int main(int argc, const char *args[])
{
	try
	{
		if (argc == 3 && String(args[1]) == "analyze")
		{
			initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/analyzeLog/path", "analyze-log"), pathReplaceInvalidCharacters(args[2]) + ".log"));
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "analyzing input '" + args[2] + "'");
			inputDirectory = pathExtractDirectory(args[2]);
			inputName = pathExtractFilename(args[2]);
			derivedProperties();
			return processAnalyze();
		}

		if (argc != 2)
			CAGE_THROW_ERROR(Exception, "missing asset type parameter");

		loadProperties();
		initializeSecondaryLog(pathJoin(configGetString("cage-asset-processor/processLog/path", "process-log"), pathReplaceInvalidCharacters(inputName) + ".log"));

		for (const auto &it : props)
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "property '" + it.first + "': '" + it.second + "'");

#define GCHL_GENERATE(N) CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input " #N ": '" + N + "'");
		GCHL_GENERATE(inputFileName);
		GCHL_GENERATE(outputFileName);
		GCHL_GENERATE(inputFile);
		GCHL_GENERATE(inputSpec);
		GCHL_GENERATE(inputIdentifier);
#undef GCHL_GENERATE

		Delegate<void()> func;
		String component = String(args[1]);
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
		else if (component == "model")
			func.bind<&processModel>();
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
