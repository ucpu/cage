#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

using namespace cage;

void info(const String &src)
{
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "opening image '" + src + "'");
	Holder<Image> img = newImage();
	try
	{
		img->importFile(src);
	}
	catch (const Exception &)
	{
		return;
	}
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "resolution: " + img->width() + "x" + img->height());
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "total channels: " + img->channels());
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "color channels: " + img->colorConfig.colorChannelsCount);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "gamma space: " + detail::imageGammaSpaceToString(img->colorConfig.gammaSpace));
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "alpha mode: " + detail::imageAlphaModeToString(img->colorConfig.alphaMode));
	if (img->colorConfig.alphaChannelIndex != m)
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "alpha channel index: " + img->colorConfig.alphaChannelIndex);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "format: " + detail::imageFormatToString(img->format()));
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
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		for (const String &path : paths)
			info(path);
		CAGE_LOG(SeverityEnum::Info, "image", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
