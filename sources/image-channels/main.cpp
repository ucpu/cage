#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/string.h>

using namespace cage;

constexpr uint32 MaxChannels = 8;

void doSplit(const String names[MaxChannels], const String &input)
{
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: " + input);
	Holder<Image> in = newImage();
	in->importFile(input);
	CAGE_LOG(SeverityEnum::Info, "image", "splitting");
	const auto images = imageChannelsSplit(+in);
	uint32 outputsCount = 0;
	for (uint32 index = 0; index < MaxChannels; index++)
	{
		if (names[index].empty())
			continue;
		if (index >= images.size())
		{
			CAGE_LOG_THROW(Stringizer() + "requested channel index: " + index);
			CAGE_THROW_ERROR(Exception, "input image does not have specified channel");
		}
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: " + names[index]);
		CAGE_ASSERT(images[index]->channels() == 1);
		images[index]->exportFile(names[index]);
		outputsCount++;
	}
	if (outputsCount == 0)
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
	const Holder<Image> black = newImage();
	const Holder<Image> white = newImage();

	Holder<Image> inputs[MaxChannels];
	uint32 highest = 0;
	uint32 inputsCount = 0;
	for (uint32 index = 0; index < MaxChannels; index++)
	{
		if (names[index].empty())
			continue;
		highest = index;
		if (names[index] == "0" || names[index] == "black")
		{
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "using zero (black), for " + (index + 1) + "th channel");
			inputs[index] = black.share();
			continue;
		}
		if (names[index] == "1" || names[index] == "white")
		{
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "using one (white), for " + (index + 1) + "th channel");
			inputs[index] = white.share();
			continue;
		}
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "loading image: " + names[index] + ", for " + (index + 1) + "th channel");
		inputs[index] = newImage();
		inputs[index]->importFile(names[index]);
		if (inputs[index]->channels() != 1)
		{
			if (!mono)
				CAGE_THROW_ERROR(Exception, "the image has to be mono channel");
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "monochromatizing");
			inputs[index] = monochromatize(+inputs[index]);
		}
		inputsCount++;
	}
	if (inputsCount == 0)
		CAGE_THROW_ERROR(Exception, "no inputs specified");

	{
		Vec2i res;
		for (auto &it : inputs)
		{
			if (!it)
				continue;
			if (+it == +black || +it == +white)
				continue;
			res = it->resolution();
			break;
		}
		if (res != Vec2i())
		{
			black->initialize(res, 1);
			imageFill(+black, 0);
			white->initialize(res, 1);
			imageFill(+white, 1);
		}
	}

	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "joining");
	Holder<Image> image = imageChannelsJoin(inputs);
	imageConvert(+image, highest + 1);
	CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "saving image: " + output);
	image->exportFile(output);
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
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
			String input = cmd->cmdString('i', "input");

			if (!help)
			{
				cmd->checkUnusedWithHelp();
				doSplit(names, input);
				return 0;
			}
		}

		if (join && !split)
		{
			for (uint32 i = 0; i < MaxChannels; i++)
				names[i] = cmd->cmdString(0, Stringizer() + (i + 1), names[i]);
			const String output = cmd->cmdString('o', "output");
			const bool mono = cmd->cmdBool('m', "mono", false);

			if (!help)
			{
				cmd->checkUnusedWithHelp();
				doJoin(names, output, mono);
				return 0;
			}
		}

		if (help)
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -j -1 r.png -2 g.png -o rg.png");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " -s -i rg.png -1 r.png -2 g.png");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "predefined 'black' and 'white' images can be used for joining");
			return 0;
		}

		CAGE_THROW_ERROR(Exception, "exactly one of --split (-s) or --join (-j) has to be specified");
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
