#include <map>
#include <set>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/filesystem.h>
#include <cage-core/config.h>
#include <cage-core/cmdOptions.h>
#include <cage-core/program.h>

using namespace cage;

namespace
{
	typedef std::map<string, std::set<string>> assetsListType;
	
	bool recursive = false;

	void processAnalyze(const string &analyzator, assetsListType &assets)
	{
		holder<programClass> prg = newProgram(analyzator);
		bool ignore = true;
		string scheme;
		while (true)
		{
			string line = prg->readLine();
			CAGE_LOG_DEBUG(severityEnum::Info, "generator", string() + "message '" + line+ "'");
			if (line == "cage-stop")
				break;
			else if (line == "cage-begin")
				ignore = false;
			else if (line == "cage-end")
				ignore = true;
			else if (!ignore)
			{
				string a = line.split("=").trim();
				string b = line.trim();
				if (a == "scheme")
					scheme = b;
				else if (a == "asset")
					assets[scheme].insert(b);
				else
					CAGE_THROW_ERROR(exception, "invalid analyze tool output");
			}
		}
	}

	void processFile(const string &path, assetsListType &assets)
	{
		CAGE_LOG(severityEnum::Info, "generator", string() + "processing file '" + path + "'");
		try
		{
			processAnalyze(string() + "cage-asset-processor analyze " + path, assets);
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Warning, "generator", string() + "an error ocurred while processing file '" + path + "'");
		}
	}

	void processFolder(const string &path)
	{
		CAGE_LOG(severityEnum::Info, "generator", string() + "processing directory '" + path + "'");
		std::set<string> files, directories;
		holder<directoryListClass> dl = newDirectoryList(path);
		while (dl->valid())
		{
			string p = dl->name();
			if (dl->isDirectory())
				directories.insert(p);
			else
				files.insert(p);
			dl->next();
		}
		assetsListType assets;
		for (auto &f : files)
			processFile(pathJoin(path, f), assets);
		if (!assets.empty())
		{
			CAGE_LOG(severityEnum::Info, "generator", "generating assets");
			holder<fileClass> f1 = newFile(pathJoin(path, "generated.asset"), fileMode(false, true, true));
			holder<fileClass> f2 = newFile(pathJoin(path, "generated.pack"), fileMode(false, true, true));
			for (auto &a : assets)
			{
				f1->writeLine("[]");
				f2->writeLine("[]");
				f1->writeLine(string() + "scheme = " + a.first);
				for (auto b : a.second)
				{
					if (b.find('=') != -1)
						b = string() + "=" + b;
					f1->writeLine(b);
					f2->writeLine(b);
				}
			}
			f1->writeLine("[]");
			f1->writeLine("scheme = pack");
			f1->writeLine("generated.pack");
		}
		else
		{
			CAGE_LOG(severityEnum::Info, "generator", "directory does not contain any recognized assets");
		}
		if (recursive)
		{
			for (auto d : directories)
				processFolder(pathJoin(path, d));
		}
	}
}

int main(int argc, const char *args[])
{
	holder<loggerClass> conLog = newLogger();
	conLog->format.bind<&logFormatPolicyConsole>();
	conLog->output.bind<&logOutputPolicyStdOut>();

	try
	{
		string dir;
		holder<cmdOptionsClass> cso = newCmdOptions(argc, args, "rd:");
		while (cso->next())
		{
			switch (cso->option())
			{
			case 'r':
				recursive = true;
				break;
			case 'd':
				dir = cso->argument();
				break;
			}
		}
		dir = pathToAbs(dir);
		if (!pathIsDirectory(dir))
			CAGE_THROW_ERROR(exception, "directory does not exist");
		processFolder(dir);
	}
	catch (const cage::exception &)
	{
		return 1;
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Error, "exception", e.what());
		return 1;
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown error");
		return 1;
	}
	return 0;
}
