#include <cage-core/logger.h>
#include <cage-core/files.h>
#include <cage-core/config.h>
#include <cage-core/ini.h>
#include <cage-core/process.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>

#include <map>
#include <set>

using namespace cage;

namespace
{
	typedef std::map<String, std::set<String>> AssetsLists; // scheme -> assets
	
	void mergeLists(AssetsLists &out, const AssetsLists &in)
	{
		for (const auto &a : in)
		{
			for (const auto &b : a.second)
				out[a.first].insert(b);
		}
	}

	bool recursive = false;
	bool generateObjects = false;
	bool generatePacks = false;

	AssetsLists analyzeFile(const String &path)
	{
		CAGE_LOG(SeverityEnum::Info, "analyze", String() + "analyzing file '" + path + "'");
		try
		{
			Holder<Process> prg = newProcess(String() + "cage-asset-processor analyze " + path);

			AssetsLists assets;
			bool ignore = true;
			String scheme;
			while (true)
			{
				String line;
				{
					detail::OverrideBreakpoint ob;
					line = prg->readLine();
				}
				CAGE_LOG_DEBUG(SeverityEnum::Info, "process", Stringizer() + "message '" + line + "'");
				if (line == "cage-stop")
					break;
				else if (line == "cage-begin")
					ignore = false;
				else if (line == "cage-end")
					ignore = true;
				else if (!ignore)
				{
					String a = trim(split(line, "="));
					String b = trim(line);
					if (a == "scheme")
						scheme = b;
					else if (a == "asset")
						assets[scheme].insert(b);
					else
						CAGE_THROW_ERROR(Exception, "invalid analyze tool output");
				}
			}

			if (generateObjects && assets.count("model") > 0)
			{
				String name = pathExtractFilenameNoExtension(path) + ".object";
				Holder<File> f = newFile(pathJoin(pathExtractDirectory(path), name), FileMode(false, true));
				f->writeLine("[]");
				for (const auto &a : assets["model"])
					f->writeLine(a);
				f->close();
				assets["object"].insert(name);
			}

			return assets;
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Warning, "analyze", String() + "an error occurred while analyzing file '" + path + "'");
		}
		return {};
	}

	void analyzeFolder(const String &path)
	{
		CAGE_LOG(SeverityEnum::Info, "analyze", String() + "analyzing directory '" + path + "'");

		std::set<String> files, directories;
		Holder<DirectoryList> dl = newDirectoryList(path);
		while (dl->valid())
		{
			String p = dl->name();
			if (dl->isDirectory())
				directories.insert(p);
			else
				files.insert(p);
			dl->next();
		}

		if (recursive)
		{
			for (const auto &d : directories)
				analyzeFolder(pathJoin(path, d));
		}

		AssetsLists assets;
		for (auto &f : files)
			mergeLists(assets, analyzeFile(pathJoin(path, f)));

		if (assets.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "analyze", "directory does not contain any recognized assets");
		}
		else
		{
			if (generatePacks)
			{
				Holder<File> f = newFile(pathJoin(path, "analyzed.pack"), FileMode(false, true));
				f->writeLine("[]");
				for (const auto &a : assets)
				{
					for (const auto &b : a.second)
						f->writeLine(b);
				}
				f->close();
				assets["pack"].insert("analyzed.pack");
			}

			Holder<File> f = newFile(pathJoin(path, "analyzed.assets"), FileMode(false, true));
			for (const auto &a : assets)
			{
				f->writeLine("[]");
				f->writeLine(String() + "scheme = " + a.first);
				for (const auto &b : a.second)
					f->writeLine(b);
			}
			f->close();
		}
	}
}

int main(int argc, const char *args[])
{
	Holder<Logger> log = newLogger();
	log->format.bind<logFormatConsole>();
	log->output.bind<logOutputStdOut>();

	try
	{
		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		const auto &paths = cmd->cmdArray(0, "--");
		recursive = cmd->cmdBool('r', "recursive", false);
		generateObjects = cmd->cmdBool('o', "objects", false);
		generatePacks = cmd->cmdBool('p', "packs", false);
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " path1 path2 path3");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " --recursive -- path");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " --objects -- path");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " --packs -- path");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no input");
		for (const String &path : paths)
			analyzeFolder(pathToAbs(path));
		CAGE_LOG(SeverityEnum::Info, "analyze", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
