#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

using namespace cage;

bool preserveOriginal = false;

void convert(string src, const string &format)
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
	{ // encode to buffer first to verify that the conversion is possible
		MemoryBuffer buf = img->exportBuffer(format);
		Holder<File> f = writeFile(dst);
		f->write(buf);
		f->close();
	}
	if (!preserveOriginal)
		pathRemove(src);
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
		if (cmd->cmdBool('?', "help", false) || paths.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "example:");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " --preserve --format .jpg -- a.png b.tiff");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "  to convert a.png to a.jpg and b.tiff to b.jpg");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "--preserve will keep original files too");
			return 0;
		}
		preserveOriginal = cmd->cmdBool('p', "preserve", false);
		string format = cmd->cmdString('f', "format");
		cmd->checkUnused();
		for (const string &path : paths)
			convert(path, format);
		CAGE_LOG(SeverityEnum::Info, "image", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
