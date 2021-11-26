#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/string.h>

using namespace cage;

void doSplit(const String names[4], const String &input)
{
	{
		uint32 outputs = 0;
		for (uint32 index = 0; index < 4; index++)
			if (!names[index].empty())
				outputs++;
		if (outputs == 0)
			CAGE_THROW_ERROR(Exception, "no outputs specified");
	}

	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: '" + input + "'");
	Holder<Image> in = newImage();
	in->importFile(input);
	if (in->format() == ImageFormatEnum::Rgbe)
		CAGE_THROW_ERROR(Exception, "input image uses Rgbe format, which cannot be split");
	const uint32 width = in->width();
	const uint32 height = in->height();
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "resolution: " + width + "x" + height + ", channels: " + in->channels());

	Holder<Image> out = newImage();
	for (uint32 ch = 0; ch < in->channels(); ch++)
	{
		if (names[ch].empty())
			continue;
		out->initialize(width, height, 1, in->format());
		for (uint32 y = 0; y < height; y++)
		{
			for (uint32 x = 0; x < width; x++)
				out->value(x, y, 0, in->value(x, y, ch));
		}
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: '" + names[ch] + "'");
		out->exportFile(names[ch]);
	}
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
}

void doJoin(const String names[4], const String &output, const bool mono)
{
	Holder<Image> pngs[4];
	uint32 width = 0, height = 0;
	uint32 channels = 0;
	for (uint32 index = 0; index < 4; index++)
	{
		const String name = names[index];
		if (!name.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: '" + name + "' for " + (index + 1) + "th channel");
			Holder<Image> p = newImage();
			p->importFile(name);
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "resolution: " + p->width() + "x" + p->height() + ", channels: " + p->channels());
			if (width == 0)
			{
				width = p->width();
				height = p->height();
			}
			else
			{
				if (p->width() != width || p->height() != height)
					CAGE_THROW_ERROR(Exception, "image resolution does not match");
			}
			if (p->channels() != 1)
			{
				if (!mono)
					CAGE_THROW_ERROR(Exception, "the image has to be mono channel");
				CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "monochromatizing");
				Holder<Image> m = newImage();
				m->initialize(width, height, 1);
				uint32 ch = p->channels();
				for (uint32 y = 0; y < height; y++)
				{
					for (uint32 x = 0; x < width; x++)
					{
						float sum = 0;
						for (uint32 c = 0; c < ch; c++)
							sum += p->value(x, y, c);
						m->value(x, y, 0, sum / ch);
					}
				}
				p = std::move(m);
			}
			channels = max(channels, index + 1u);
			pngs[index] = std::move(p);
		}
	}
	if (channels == 0)
		CAGE_THROW_ERROR(Exception, "no inputs specified");

	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "joining image");
	Holder<Image> res = newImage();
	res->initialize(width, height, channels);
	for (uint32 i = 0; i < channels; i++)
	{
		Holder<Image> &src = pngs[i];
		if (!src)
			continue;
		for (uint32 y = 0; y < height; y++)
		{
			for (uint32 x = 0; x < width; x++)
				res->value(x, y, i, src->value(x, y, 0));
		}
	}

	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: '" + output + "'");
	res->exportFile(output);
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
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
		const bool split = cmd->cmdBool('s', "split", false);
		const bool join = cmd->cmdBool('j', "join", false);
		const bool help = cmd->cmdBool('?', "help", false);

		if (split && !join)
		{
			String names[4] = { "", "", "", "" };
			for (uint32 i = 0; i < 4; i++)
				names[i] = cmd->cmdString(0, Stringizer() + (i + 1), names[i]);
			String input = cmd->cmdString('i', "input", "input.png");

			if (!help)
			{
				cmd->checkUnusedWithHelp();
				doSplit(names, input);
			}
		}

		if (join && !split)
		{
			String names[4] = { "", "", "", "" };
			for (uint32 i = 0; i < 4; i++)
				names[i] = cmd->cmdString(0, Stringizer() + (i + 1), names[i]);
			const String output = cmd->cmdString('o', "output", "output.png");
			const bool mono = cmd->cmdBool('m', "mono", false);

			if (!help)
			{
				cmd->checkUnusedWithHelp();
				doJoin(names, output, mono);
			}
		}

		if (help)
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -j -1 r.png -2 g.png -o rg.png");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -s -i rg.png -1 r.png -2 g.png");
			return 0;
		}

		if (join == split)
			CAGE_THROW_ERROR(Exception, "exactly one of --split (-s) and --join (-j) has to be specified");

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
