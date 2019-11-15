#include <exception>

#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/configIni.h>

using namespace cage;

void separate(holder<configIni> &cmd)
{
	string names[4] = { "1.png", "2.png", "3.png", "4.png" };
	string input = "input.png";
	for (uint32 i = 0; i < 4; i++)
		names[i] = cmd->cmdString(0, string(i + 1), names[i]);
	input = cmd->cmdString('i', "input", input);
	cmd->checkUnused();

	CAGE_LOG(severityEnum::Info, "image", stringizer() + "loading image: '" + input + "'");
	holder<image> in = newImage();
	in->decodeFile(input);
	uint32 width = in->width();
	uint32 height = in->height();
	CAGE_LOG(severityEnum::Info, "image", stringizer() + "image resolution: " + width + "x" + height + ", channels: " + in->channels());

	holder<image> out = newImage();
	for (uint32 ch = 0; ch < in->channels(); ch++)
	{
		if (names[ch].empty())
			continue;
		out->empty(width, height, 1);
		for (uint32 y = 0; y < height; y++)
		{
			for (uint32 x = 0; x < width; x++)
				out->value(x, y, 0, in->value(x, y, ch));
		}
		CAGE_LOG(severityEnum::Info, "image", stringizer() + "saving image: '" + names[ch] + "'");
		out->encodeFile(names[ch]);
	}
	CAGE_LOG(severityEnum::Info, "image", "ok");
}

void combine(holder<configIni> &cmd)
{
	string names[4] = { "", "", "", "" };
	string output = "output.png";
	for (uint32 i = 0; i < 4; i++)
		names[i] = cmd->cmdString(0, string(i + 1), names[i]);
	output = cmd->cmdString('o', "output", output);
	cmd->checkUnused();

	holder<image> pngs[4];
	uint32 width = 0, height = 0;
	uint32 channels = 0;
	for (uint32 index = 0; index < 4; index++)
	{
		string name = names[index];
		if (!name.empty())
		{
			CAGE_LOG(severityEnum::Info, "image", stringizer() + "loading image: '" + name + "' for " + (index + 1) + "th channel");
			holder<image> p = newImage();
			p->decodeFile(name);
			CAGE_LOG(severityEnum::Info, "image", stringizer() + "image resolution: " + p->width() + "x" + p->height() + ", channels: " + p->channels());
			if (width == 0)
			{
				width = p->width();
				height = p->height();
			}
			else
			{
				if (p->width() != width || p->height() != height)
					CAGE_THROW_ERROR(exception, "image resolution does not match");
			}
			if (p->channels() != 1)
				CAGE_THROW_ERROR(exception, "the image has to be mono channel");
			channels = max(channels, index + 1u);
			pngs[index] = templates::move(p);
		}
	}
	if (channels == 0)
		CAGE_THROW_ERROR(exception, "no inputs specified");

	CAGE_LOG(severityEnum::Info, "image", stringizer() + "combining image");
	holder<image> res = newImage();
	res->empty(width, height, channels);
	for (uint32 i = 0; i < channels; i++)
	{
		holder<image> &src = pngs[i];
		if (!src)
			continue;
		for (uint32 y = 0; y < height; y++)
		{
			for (uint32 x = 0; x < width; x++)
				res->value(x, y, i, src->value(x, y, 0));
		}
	}

	CAGE_LOG(severityEnum::Info, "image", stringizer() + "saving image: '" + output + "'");
	res->encodeFile(output);
	CAGE_LOG(severityEnum::Info, "image", "ok");
}

int main(int argc, const char *args[])
{
	try
	{
		holder<logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		holder<configIni> cmd = newConfigIni();
		cmd->parseCmd(argc, args);
		if (cmd->cmdBool('s', "separate", false))
			separate(cmd);
		else
			combine(cmd);
		return 0;
	}
	catch (const cage::exception &)
	{
		// nothing
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Error, "exception", stringizer() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
