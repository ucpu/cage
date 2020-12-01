#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/files.h>
#include <cage-core/string.h>

using namespace cage;

void cutImage(const Holder<Image> &in, uint32 x, uint32 y, uint32 w, uint32 h, const string &name)
{
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "cutting image at: " + x + ", " + y);
	uint32 tc = in->channels();
	Holder<Image> out = newImage();
	imageBlit(in.get(), out.get(), x, y, 0, 0, w, h);
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "saving to: '" + name + "'");
	out->exportFile(name);
}

void doAtlas(const string &input, const string &output, uint32 x, uint32 y, uint32 w, uint32 h)
{
	if (w == 0 || h == 0)
		CAGE_THROW_ERROR(Exception, "width and height may not be zero");

	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "loading image: '" + input + "'");
	Holder<Image> in = newImage();
	in->importFile(input);
	uint32 inw = in->width();
	uint32 inh = in->height();
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "image resolution: " + inw + "x" + inh + ", channels: " + in->channels());

	if (x >= inw || y >= inh)
		CAGE_THROW_ERROR(Exception, "x or y offsets are outside the image");
	uint32 cx = (inw - x) / w;
	uint32 cy = (inh - y) / h;
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "output counts: " + cx + "x" + cy);
	if (cx == 0 || cy == 0)
		CAGE_THROW_ERROR(Exception, "no output images");

	string prefix, suffix;
	uint32 dollarsCount = 0;
	{ // identify output files name format
		uint32 firstDollar = find(output, '$');
		if (firstDollar == m)
		{
			suffix = pathExtractExtension(output);
			prefix = subString(output, 0, output.length() - suffix.length());
		}
		else
		{
			prefix = subString(output, 0, firstDollar);
			suffix = subString(output, firstDollar, m);
			dollarsCount = 0;
			while (!suffix.empty() && suffix[0] == '$')
			{
				dollarsCount++;
				suffix = subString(suffix, 1, m);
			}
		}
	}

	{ // cut the atlas
		uint32 count = cx * cy;
		uint32 index = 0;
		while (index < count)
		{
			string n = stringizer() + index;
			n = reverse(fill(reverse(n), dollarsCount, '0'));
			string name = prefix + n + suffix;
			cutImage(in, x + (index % cx) * w, y + (index / cx) * h, w, h, name);
			index++;
		}
	}

	CAGE_LOG(SeverityEnum::Info, "image", "ok");
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
		const string input = cmd->cmdString('i', "input", "input.png");
		const string output = cmd->cmdString('o', "output", "output-$$$$.png");
		const uint32 x = cmd->cmdUint32('x', "offset-x", 0);
		const uint32 y = cmd->cmdUint32('y', "offset-y", 0);
		const uint32 w = cmd->cmdUint32('w', "width", 0);
		const uint32 h = cmd->cmdUint32('h', "height", 0);
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", stringizer() + args[0] + " -i atlas.png -x 13 -y 42 -w 32 -h 32 -o frame-$$.png");
			return 0;
		}
		cmd->checkUnusedWithHelp();

		doAtlas(input, output, x, y, w, h);
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
