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
	CAGE_LOG(SeverityEnum::Info, "imageResize", Stringizer() + "resizing file: " + name);
	Holder<Image> img = newImage();
	img->importFile(name);
	CAGE_LOG(SeverityEnum::Info, "imageResize", Stringizer() + "original resolution: " + img->width() + "x" + img->height() + ", channels: " + img->channels());
	imageResize(+img, resolution);
	img->exportFile(name);
}

int main(int argc, const char *args[])
{
	try
	{
		initializeConsoleLogger();

		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		cmd->addHelp(Stringizer() + "example:");
		cmd->addHelp(Stringizer() + args[0] + " -w 512 -h 1024 -- a.png b.tiff");
		cmd->addHelp("");
		Vec2i resolution;
		resolution[0] = cmd->cmdUint32('w', "width", 0);
		resolution[1] = cmd->cmdUint32('h', "height", 0);
		const auto paths = cmd->cmdArray(0, "--");
		cmd->checkCmd();

		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		if (resolution[0] <= 0 || resolution[1] <= 0)
			CAGE_THROW_ERROR(Exception, "invalid resolution");
		for (const String &path : paths)
			resize(path, resolution);
		CAGE_LOG(SeverityEnum::Info, "imageResize", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
