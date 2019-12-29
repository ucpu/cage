#include <exception>

#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>

using namespace cage;

void doSplit(Holder<Ini> &cmd)
{
	string names[4] = { "", "", "", "" };
	string input = "input.png";
	for (uint32 i = 0; i < 4; i++)
		names[i] = cmd->cmdString(0, string(i + 1), names[i]);
	input = cmd->cmdString('i', "input", input);
	cmd->checkUnused();

	{
		uint32 outputs = 0;
		for (const string &n : names)
			if (!n.empty())
				outputs++;
		if (outputs == 0)
			CAGE_THROW_ERROR(Exception, "no outputs specified");
	}

	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "loading image: '" + input + "'");
	Holder<Image> in = newImage();
	in->decodeFile(input);
	uint32 width = in->width();
	uint32 height = in->height();
	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "image resolution: " + width + "x" + height + ", channels: " + in->channels());

	Holder<Image> out = newImage();
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
		CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "saving image: '" + names[ch] + "'");
		out->encodeFile(names[ch]);
	}
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
}

void doJoin(Holder<Ini> &cmd)
{
	string names[4] = { "", "", "", "" };
	string output = "output.png";
	for (uint32 i = 0; i < 4; i++)
		names[i] = cmd->cmdString(0, string(i + 1), names[i]);
	output = cmd->cmdString('o', "output", output);
	bool autoMono = cmd->cmdBool('m', "mono", false);
	cmd->checkUnused();

	Holder<Image> pngs[4];
	uint32 width = 0, height = 0;
	uint32 channels = 0;
	for (uint32 index = 0; index < 4; index++)
	{
		string name = names[index];
		if (!name.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "loading image: '" + name + "' for " + (index + 1) + "th channel");
			Holder<Image> p = newImage();
			p->decodeFile(name);
			CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "image resolution: " + p->width() + "x" + p->height() + ", channels: " + p->channels());
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
				if (!autoMono)
					CAGE_THROW_ERROR(Exception, "the image has to be mono channel");
				CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "monochromatizing");
				Holder<Image> m = newImage();
				m->empty(width, height, 1);
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
				p = templates::move(m);
			}
			channels = max(channels, index + 1u);
			pngs[index] = templates::move(p);
		}
	}
	if (channels == 0)
		CAGE_THROW_ERROR(Exception, "no inputs specified");

	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "joining image");
	Holder<Image> res = newImage();
	res->empty(width, height, channels);
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

	CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "saving image: '" + output + "'");
	res->encodeFile(output);
	CAGE_LOG(SeverityEnum::Info, "image", "ok");
}

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		Holder<Ini> cmd = newConfigIni();
		cmd->parseCmd(argc, args);
		if (cmd->cmdBool('h', "help", false))
		{
			CAGE_LOG(SeverityEnum::Info, "image", stringizer() + "examples:");
			CAGE_LOG(SeverityEnum::Info, "image", stringizer() + args[0] + " -j -1 r.png -2 g.png -o rg.png");
			CAGE_LOG(SeverityEnum::Info, "image", stringizer() + args[0] + " -s -i rg.png -1 r.png -2 g.png");
			return 0;
		}
		bool split = cmd->cmdBool('s', "split", false);
		bool join = cmd->cmdBool('j', "join", false);
		if (join == split)
			CAGE_THROW_ERROR(Exception, "exactly one of -s (--split) and -j (--join) has to be specified");
		if (split)
			doSplit(cmd);
		if (join)
			doJoin(cmd);
		return 0;
	}
	catch (const cage::Exception &)
	{
		// nothing
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(SeverityEnum::Error, "exception", stringizer() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(SeverityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
