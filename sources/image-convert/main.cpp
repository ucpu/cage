#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

using namespace cage;

void convert(string src, const string &format, bool preserveOriginal)
{
	src = pathSimplify(src);
	string path = pathExtractDirectory(src);
	string dst = pathJoin(path, pathExtractFilenameNoExtension(src) + format);
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "converting '" + src + "' to '" + dst + "'");
	if (src == dst)
	{
		CAGE_LOG(SeverityEnum::Info, "image", "no conversion required");
		return;
	}
	Holder<Image> img = newImage();
	img->importFile(src);
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "width: " + img->width());
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "height: " + img->height());
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "channels: " + img->channels());
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "format: " + (uint32)img->format());
	img->exportFile(dst);
	if (!preserveOriginal)
		pathRemove(src);
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
		const bool preserveOriginal = cmd->cmdBool('p', "preserve", false);
		const string format = cmd->cmdString('f', "format", ".png");
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "example:");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " --preserve --format .jpg -- a.png b.tiff");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "  to convert a.png to a.jpg and b.tiff to b.jpg");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "--preserve will keep original files too");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		for (const string &path : paths)
			convert(path, format, preserveOriginal);
		CAGE_LOG(SeverityEnum::Info, "image", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
