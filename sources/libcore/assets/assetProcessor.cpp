#include <cstdio> // fgets, ferror
#include <cstring> // strlen

#include <unordered_dense.h>

#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetProcessor.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
		class AssetProcessorImpl : public AssetProcessor
		{
		public:
			ankerl::unordered_dense::map<String, String> props;

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

				inputDirectory = property("inputDirectory");
				inputName = property("inputName");
				outputDirectory = property("outputDirectory");
				outputName = toUint32(property("outputName"));
				schemeIndex = toUint32(property("schemeIndex"));

				derivedProperties();
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
				outputFileName = pathJoin(outputDirectory, Stringizer() + outputName);
			}

			String convertGenericPath(const String &input_, const String &relativeTo_) const
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

			static String separateDetail(String &p)
			{
				const uint32 sep = min(find(p, '?'), find(p, ';'));
				const String detail = subString(p, sep, m);
				p = subString(p, 0, sep);
				return detail;
			}
		};
	}

	void AssetProcessor::loadProperties()
	{
		AssetProcessorImpl *impl = (AssetProcessorImpl *)this;
		impl->loadProperties();
	}

	void AssetProcessor::derivedProperties()
	{
		AssetProcessorImpl *impl = (AssetProcessorImpl *)this;
		impl->derivedProperties();
	}

	void AssetProcessor::logProperties() const
	{
		const AssetProcessorImpl *impl = (const AssetProcessorImpl *)this;
		for (const auto &it : impl->props)
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "property: " + it.first + " = " + it.second);

#define GCHL_GENERATE(N) CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input: " #N " = " + N);
		GCHL_GENERATE(inputFileName);
		GCHL_GENERATE(outputFileName);
		GCHL_GENERATE(inputFile);
		GCHL_GENERATE(inputSpec);
		GCHL_GENERATE(inputIdentifier);
#undef GCHL_GENERATE
	}

	String AssetProcessor::property(const String &name) const
	{
		const AssetProcessorImpl *impl = (const AssetProcessorImpl *)this;
		auto it = impl->props.find(name);
		if (it != impl->props.end())
			return it->second;
		else
		{
			CAGE_LOG_THROW(Stringizer() + "property name: " + name);
			CAGE_THROW_ERROR(Exception, "property not found");
		}
	}

	String AssetProcessor::readLine()
	{
		char buf[String::MaxLength];
		if (std::fgets(buf, String::MaxLength, stdin) == nullptr)
			CAGE_THROW_ERROR(SystemError, "fgets", std::ferror(stdin));
		return trim(String(buf));
	}

	void AssetProcessor::writeLine(const String &other)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "writing: " + other);
		{
			String b = other;
			if (trim(split(b, "=")) == "ref")
				CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "reference hash: " + (uint32)HashString(trim(b)));
		}
		if (fprintf(stdout, "%s\n", other.c_str()) < 0)
			CAGE_THROW_ERROR(SystemError, "fprintf", ferror(stdout));
	}

	void AssetProcessor::initializeSecondaryLog(const String &path)
	{
		static Holder<LoggerOutputFile> *secondaryLogFile = new Holder<LoggerOutputFile>(); // intentional leak
		static Holder<Logger> *secondaryLog = new Holder<Logger>(); // intentional leak - this will allow to log to the very end of the application
		*secondaryLogFile = newLoggerOutputFile(path, false);
		*secondaryLog = newLogger();
		(*secondaryLog)->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(secondaryLogFile->get());
		(*secondaryLog)->format.bind<logFormatFileShort>();
	}

	String AssetProcessor::convertAssetPath(const String &input, const String &relativeTo, bool markAsReferenced) const
	{
		const AssetProcessorImpl *impl = (const AssetProcessorImpl *)this;
		String p = input;
		const String detail = impl->separateDetail(p);
		p = impl->convertGenericPath(p, relativeTo) + detail;
		if (markAsReferenced)
			writeLine(String("ref=") + p);
		return p;
	}

	String AssetProcessor::convertFilePath(const String &input, const String &relativeTo, bool markAsUsed) const
	{
		const AssetProcessorImpl *impl = (const AssetProcessorImpl *)this;
		String p = input;
		const String detail = impl->separateDetail(p);
		p = impl->convertGenericPath(p, relativeTo);
		if (markAsUsed)
			writeLine(String("use=") + p);
		return p + detail;
	}

	AssetHeader AssetProcessor::initializeAssetHeader() const
	{
		return AssetHeader(inputName, numeric_cast<uint16>(schemeIndex));
	}

	Holder<AssetProcessor> newAssetProcessor()
	{
		return systemMemory().createImpl<AssetProcessor, AssetProcessorImpl>();
	}
}
