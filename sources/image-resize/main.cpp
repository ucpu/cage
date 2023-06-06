#include <cage-core/files.h>
#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/string.h>

using namespace cage;

void resize(String name, const Vec2i resolution)
{
	name = pathSimplify(name);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "resizing file: " + name);
	Holder<Image> img = newImage();
	img->importFile(name);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "original resolution: " + img->width() + "x" + img->height() + ", channels: " + img->channels());
	imageResize(+img, resolution);
	img->exportFile(name);
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
		Vec2i resolution;
		resolution[0] = cmd->cmdUint32('w', "width");
		resolution[1] = cmd->cmdUint32('h', "height");
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "example:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -w 512 -h 1024 -- a.png b.tiff");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		for (const String &path : paths)
			resize(path, resolution);
		CAGE_LOG(SeverityEnum::Info, "image", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
