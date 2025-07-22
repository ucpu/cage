#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>

using namespace cage;

void performList(PointerRange<const String> paths)
{
	for (const String &p : paths)
	{
		if (paths.size() > 1)
			CAGE_LOG(SeverityEnum::Info, "files", Stringizer() + p + ":");
		const auto contents = pathListDirectory(p);
		for (const String &it : contents)
			CAGE_LOG(SeverityEnum::Info, "files", Stringizer() + pathToRel(it, p));
		CAGE_LOG(SeverityEnum::Info, "files", "");
	}
}

void performMove(PointerRange<const String> paths)
{
	if (paths.size() != 2)
		CAGE_THROW_ERROR(Exception, "move requires exactly two paths");
	pathMove(paths[0], paths[1]);
}

void performCopy(PointerRange<const String> paths)
{
	if (paths.size() != 2)
		CAGE_THROW_ERROR(Exception, "copy requires exactly two paths");
	pathMove(paths[0], paths[1]);
}

void performRemove(PointerRange<const String> paths)
{
	for (const String &p : paths)
		pathRemove(p);
}

void performExtract(PointerRange<const String> paths)
{
	if (paths.size() != 2)
		CAGE_THROW_ERROR(Exception, "extract requires exactly two paths");
	if (none(pathType(paths[0]) & PathTypeFlags::Archive))
		CAGE_THROW_ERROR(Exception, "source path is not an archive");
	const auto dt = pathType(paths[1]);
	if (any(dt & (PathTypeFlags::Invalid | PathTypeFlags::Archive | PathTypeFlags::File)))
		CAGE_THROW_ERROR(Exception, "target path is invalid");
	CAGE_ASSERT(any(dt & (PathTypeFlags::NotFound | PathTypeFlags::Directory)));
	const auto contents = pathListDirectory(paths[0]);
	for (const String &it : contents)
	{
		const String p = pathJoin(paths[1], pathToRel(it, paths[0]));
		pathCopy(it, p);
	}
}

void performArchive(PointerRange<const String> paths)
{
	if (paths.size() != 2)
		CAGE_THROW_ERROR(Exception, "archive requires exactly two paths");
	if (none(pathType(paths[0]) & PathTypeFlags::Directory))
		CAGE_THROW_ERROR(Exception, "source path is not a directory");
	const auto dt = pathType(paths[1]);
	if (none(dt & (PathTypeFlags::Archive | PathTypeFlags::NotFound)))
		CAGE_THROW_ERROR(Exception, "target path is not an archive");
	if (any(dt & PathTypeFlags::NotFound))
		pathCreateArchiveCarch(paths[1]);
	const auto contents = pathListDirectory(paths[0]);
	for (const String &it : contents)
	{
		const String p = pathJoin(paths[1], pathToRel(it, paths[0]));
		pathCopy(it, p);
	}
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		const auto paths = cmd->cmdArray(0, "--");
		const bool list = cmd->cmdBool('l', "list", false);
		const bool move = cmd->cmdBool('m', "move", false);
		const bool copy = cmd->cmdBool('c', "copy", false);
		const bool remove = cmd->cmdBool('r', "remove", false);
		const bool extract = cmd->cmdBool('e', "extract", false);
		const bool archive = cmd->cmdBool('a', "archive", false);
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -l -- .");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -c -- assets.carch/source_file.txt extracted/target_file.txt");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -e -- source.carch target_folder");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -a -- source_folder target.carch");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if ((list + move + copy + remove + extract + archive) != 1)
			CAGE_THROW_ERROR(Exception, "exactly one operation is required");
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no paths");
		if (list)
			performList(paths);
		else if (move)
			performMove(paths);
		else if (copy)
			performCopy(paths);
		else if (remove)
			performRemove(paths);
		else if (extract)
			performExtract(paths);
		else if (archive)
			performArchive(paths);
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
