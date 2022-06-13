#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>
#include <cage-core/string.h>

using namespace cage;

constexpr uint32 MaxChannels = 8;

void doSplit(const String names[MaxChannels], const String &input)
{
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: '" + input + "'");
	Holder<Image> in = newImage();
	in->importFile(input);
	CAGE_LOG(SeverityEnum::Info, "image", "splitting");
	const auto images = imageChannelsSplit(+in);
	uint32 outputs = 0;
	for (uint32 index = 0; index < MaxChannels; index++)
	{
		if (names[index].empty())
			continue;
		if (index >= images.size())
		{
			CAGE_LOG_THROW(Stringizer() + "requested channel index: " + index);
			CAGE_THROW_ERROR(Exception, "input image does not have specified channel");
		}
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: '" + names[index] + "'");
		CAGE_ASSERT(images[index]->channels() == 1);
		images[index]->exportFile(names[index]);
		outputs++;
	}
	if (outputs == 0)
		CAGE_THROW_ERROR(Exception, "no outputs specified");
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
}

Holder<Image> monochromatize(const Image *src)
{
	Holder<Image> dst = newImage();
	dst->initialize(src->width(), src->height(), 1, src->format());
	const uint32 ch = src->channels();
	for (uint32 y = 0; y < src->height(); y++)
	{
		for (uint32 x = 0; x < src->width(); x++)
		{
			float sum = 0;
			for (uint32 c = 0; c < ch; c++)
				sum += src->value(x, y, c);
			dst->value(x, y, 0, sum / ch);
		}
	}
	return dst;
}

void doJoin(const String names[MaxChannels], const String &output, const bool mono)
{
	Holder<Image> inputs[MaxChannels];
	uint32 highest = 0;
	for (uint32 index = 0; index < MaxChannels; index++)
	{
		if (names[index].empty())
			continue;
		highest = index;
		inputs[index] = newImage();
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: '" + names[index] + "' for " + (index + 1) + "th channel");
		inputs[index]->importFile(names[index]);
		if (inputs[index]->channels() != 1)
		{
			if (!mono)
				CAGE_THROW_ERROR(Exception, "the image has to be mono channel");
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "monochromatizing");
			inputs[index] = monochromatize(+inputs[index]);
		}
	}
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "joining");
	Holder<Image> image = imageChannelsJoin(inputs);
	imageConvert(+image, highest + 1);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: '" + output + "'");
	image->exportFile(output);
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

		String names[MaxChannels] = {};

		if (split && !join)
		{
			for (uint32 i = 0; i < MaxChannels; i++)
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
			for (uint32 i = 0; i < MaxChannels; i++)
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
