#include <exception>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/image.h>
#include <cage-core/ini.h>

using namespace cage;

int main(int argc, const char *args[])
{
	try
	{
		holder<loggerClass> log1 = newLogger();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		holder<iniClass> cmd = newIni();
		cmd->parseCmd(argc, args);
		holder<imageClass> pngs[4];
		uint32 width = 0, height = 0;
		uint32 channels = 0;
		string output;
		for (string option : cmd->sections())
		{
			if (option == "1" || option == "2" || option == "3" || option == "4")
			{
				uint32 index = option.toUint32() - 1;
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				string name = cmd->get(option, "0");
				CAGE_LOG(severityEnum::Info, "combine-image", string() + "loading image: '" + name + "' for " + (index + 1) + "th channel");
				holder<imageClass> p = newImage();
				p->decodeFile(name);
				CAGE_LOG(severityEnum::Info, "combine-image", string() + "image resolution: " + p->width() + "x" + p->height() + ", channels: " + p->channels());
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
			else if (option == "o" || option == "output")
			{
				if (cmd->itemsCount(option) != 1)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
					CAGE_THROW_ERROR(exception, "option expects one argument");
				}
				output = cmd->get(option, "0");
			}
			else
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "option: '" + option + "'");
				CAGE_THROW_ERROR(exception, "unknown option");
			}
		}
		if (channels == 0)
			CAGE_THROW_ERROR(exception, "no inputs specified");
		if (output.empty())
			output = "combined.png";

		CAGE_LOG(severityEnum::Info, "combine-image", string() + "combining image");
		holder<imageClass> res = newImage();
		res->empty(width, height, channels);
		for (uint32 i = 0; i < channels; i++)
		{
			holder<imageClass> &src = pngs[i];
			if (!src)
				continue;
			for (uint32 y = 0; y < height; y++)
			{
				for (uint32 x = 0; x < width; x++)
					res->value(x, y, i, src->value(x, y, 0));
			}
		}

		CAGE_LOG(severityEnum::Info, "combine-image", string() + "saving image");
		res->encodeFile(output);

		CAGE_LOG(severityEnum::Info, "combine-image", string() + "ok");
		return 0;
	}
	catch (const cage::exception &)
	{
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Error, "exception", string() + "std exception: " + e.what());
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "unknown exception");
	}
	return 1;
}
