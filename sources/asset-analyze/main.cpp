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
	typedef std::map<string, std::set<string>> AssetsLists; // scheme -> assets
	
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

	AssetsLists analyzeFile(const string &path)
	{
		CAGE_LOG(SeverityEnum::Info, "analyze", string() + "analyzing file '" + path + "'");
		try
		{
			Holder<Process> prg = newProcess(string() + "cage-asset-processor analyze " + path);

			AssetsLists assets;
			bool ignore = true;
			string scheme;
			while (true)
			{
				string line;
				{
					detail::OverrideBreakpoint ob;
					line = prg->readLine();
				}
				CAGE_LOG_DEBUG(SeverityEnum::Info, "process", stringizer() + "message '" + line + "'");
				if (line == "cage-stop")
					break;
				else if (line == "cage-begin")
					ignore = false;
				else if (line == "cage-end")
					ignore = true;
				else if (!ignore)
				{
					string a = trim(split(line, "="));
					string b = trim(line);
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
				string name = pathExtractFilenameNoExtension(path) + ".object";
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
			CAGE_LOG(SeverityEnum::Warning, "analyze", string() + "an error occurred while analyzing file '" + path + "'");
		}
		return {};
	}

	void analyzeFolder(const string &path)
	{
		CAGE_LOG(SeverityEnum::Info, "analyze", string() + "analyzing directory '" + path + "'");

		std::set<string> files, directories;
		Holder<DirectoryList> dl = newDirectoryList(path);
		while (dl->valid())
		{
			string p = dl->name();
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
				f->writeLine(string() + "scheme = " + a.first);
				for (const auto &b : a.second)
					f->writeLine(b);
			}
			f->close();
		}
	}
}

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		const auto &paths = cmd->cmdArray(0, "--");
		recursive = cmd->cmdBool('r', "recursive", false);
		generateObjects = cmd->cmdBool('o', "objects", false);
		generatePacks = cmd->cmdBool('p', "packs", false);
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " path1 path2 path3");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " --recursive path");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " --objects path");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " --packs path");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no input");
		for (const string &path : paths)
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
