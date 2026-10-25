#include <array>

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
	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "loading image: " + input);
	Holder<Image> in = newImage();
	in->importFile(input);
	CAGE_LOG(SeverityEnum::Info, "imageChannels", "splitting");
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
		CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "saving image: " + names[index]);
		CAGE_ASSERT(images[index]->channels() == 1);
		images[index]->exportFile(names[index]);
		outputsCount++;
	}
	if (outputsCount == 0)
		CAGE_THROW_ERROR(Exception, "no outputs specified");
	CAGE_LOG(SeverityEnum::Info, "imageChannels", "done");
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

void initBlackWhite(PointerRange<const Holder<Image>> images, const Holder<Image> &black, const Holder<Image> &white)
{
	Vec2i res;
	for (auto &it : images)
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
			CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "using zero (black), for " + (index + 1) + "th channel");
			inputs[index] = black.share();
			continue;
		}
		if (names[index] == "1" || names[index] == "white")
		{
			CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "using one (white), for " + (index + 1) + "th channel");
			inputs[index] = white.share();
			continue;
		}
		CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "loading image: " + names[index] + ", for " + (index + 1) + "th channel");
		inputs[index] = newImage();
		inputs[index]->importFile(names[index]);
		if (inputs[index]->channels() != 1)
		{
			if (!mono)
				CAGE_THROW_ERROR(Exception, "the image has to be mono channel");
			CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "monochromatizing");
			inputs[index] = monochromatize(+inputs[index]);
		}
		inputsCount++;
	}
	if (inputsCount == 0)
		CAGE_THROW_ERROR(Exception, "no inputs specified");

	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "joining");
	initBlackWhite(inputs, black, white);
	Holder<Image> image = imageChannelsJoin(inputs);
	imageConvert(+image, highest + 1);
	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "saving image: " + output);
	image->exportFile(output);
	CAGE_LOG(SeverityEnum::Info, "imageChannels", "done");
}

void doReorder(const String &reorder, const String &input, const String &output)
{
	if (reorder.length() > MaxChannels)
		CAGE_THROW_ERROR(Exception, "reordering output with too many channels");

	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "loading image: " + input);
	Holder<Image> in = newImage();
	in->importFile(input);
	auto inch = imageChannelsSplit(+in);

	CAGE_LOG(SeverityEnum::Info, "imageChannels", "reordering");
	const Holder<Image> black = newImage();
	const Holder<Image> white = newImage();
	std::array<Holder<Image>, MaxChannels> channels = {};
	uint32 index = 0;
	for (const char c : reorder)
	{
		switch (c)
		{
			case 'r':
			{
				if (inch.size() < 1)
					CAGE_THROW_ERROR(Exception, "not enough input channels");
				channels[index++] = inch[0].share();
				break;
			}
			case 'g':
			{
				if (inch.size() < 2)
					CAGE_THROW_ERROR(Exception, "not enough input channels");
				channels[index++] = inch[1].share();
				break;
			}
			case 'b':
			{
				if (inch.size() < 3)
					CAGE_THROW_ERROR(Exception, "not enough input channels");
				channels[index++] = inch[2].share();
				break;
			}
			case 'a':
			{
				if (inch.size() < 4)
					CAGE_THROW_ERROR(Exception, "not enough input channels");
				channels[index++] = inch[3].share();
				break;
			}
			case '0':
			{
				channels[index++] = black.share();
				break;
			}
			case '1':
			{
				channels[index++] = white.share();
				break;
			}
			default:
			{
				CAGE_THROW_ERROR(Exception, "unknown channel identifier");
			}
		}
	}

	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "composing");
	initBlackWhite(channels, black, white);
	Holder<Image> image = imageChannelsJoin(channels);
	imageConvert(+image, reorder.length());
	CAGE_LOG(SeverityEnum::Info, "imageChannels", Stringizer() + "saving image: " + output);
	image->exportFile(output);
	CAGE_LOG(SeverityEnum::Info, "imageChannels", "done");
}

int main(int argc, const char *args[])
{
	try
	{
		initializeConsoleLogger();

		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		cmd->addHelp(Stringizer() + "examples:");
		cmd->addHelp(Stringizer() + args[0] + " -j -1 r.png -2 g.png -o rg.png");
		cmd->addHelp(Stringizer() + args[0] + " -s -i rg.png -1 r.png -2 g.png");
		cmd->addHelp(Stringizer() + args[0] + " -r bgr -i in.png -o out.png");
		cmd->addHelp(Stringizer() + "predefined 'black' (0) and 'white' (1) images can be used for joining or reordering");
		cmd->addHelp("");
		const bool split = cmd->cmdBool('s', "split", false);
		const bool join = cmd->cmdBool('j', "join", false);
		const String reorder = cmd->cmdString('r', "reorder", "");

		String names[MaxChannels] = {};
		if (split || join)
		{
			for (uint32 i = 0; i < MaxChannels; i++)
				names[i] = cmd->cmdString(0, Stringizer() + (i + 1), names[i]);
		}

		if (split && !join && reorder.empty())
		{
			const String input = cmd->cmdString('i', "input", RequiredCmdOption());
			cmd->checkCmd();
			doSplit(names, input);
			return 0;
		}

		if (join && !split && reorder.empty())
		{
			const bool mono = cmd->cmdBool('m', "mono", false);
			const String output = cmd->cmdString('o', "output", RequiredCmdOption());
			cmd->checkCmd();
			doJoin(names, output, mono);
			return 0;
		}

		if (!reorder.empty() && !join && !split)
		{
			const String input = cmd->cmdString('i', "input", RequiredCmdOption());
			const String output = cmd->cmdString('o', "output", input);
			cmd->checkCmd();
			doReorder(reorder, input, output);
			return 0;
		}

		if (cmd->requestsHelp())
			cmd->printHelp();
		CAGE_THROW_ERROR(Exception, "exactly one of --split (-s) or --join (-j) or --reorder (-r) must be specified");
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
