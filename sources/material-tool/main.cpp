#include <exception>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/png.h>
#include <cage-core/cmdOptions.h>

using namespace cage;

int main(int argc, const char *args[])
{
	try
	{
		holder<loggerClass> log1 = newLogger();
		log1->filter.bind<logFilterPolicyPass>();
		log1->format.bind<logFormatPolicyConsole>();
		log1->output.bind<logOutputPolicyStdOut>();

		holder<cmdOptionsClass> cmd = newCmdOptions(argc, args, "1:2:3:4:o:");
		holder<pngImageClass> pngs[4];
		uint32 width = 0, height = 0;
		uint32 channels = 0;
		string output;
		while (cmd->next())
		{
			switch (cmd->option())
			{
			case '1':
			case '2':
			case '3':
			case '4':
			{
				uint32 index = cmd->option() - '1';
				CAGE_LOG(severityEnum::Info, "material-tool", string() + "loading image: '" + cmd->argument() + "' for " + (index + 1) + "th channel");
				if (pngs[index])
					CAGE_THROW_ERROR(exception, "same channel specified multiple times");
				holder<pngImageClass> p = newPngImage();
				p->decodeFile(cmd->argument());
				CAGE_LOG(severityEnum::Info, "material-tool", string() + "image resolution: " + p->width() + "x" + p->height() + ", channels: " + p->channels());
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
					CAGE_THROW_ERROR(exception, "the image is not grayscale");
				channels = max(channels, index + 1u);
				pngs[index] = templates::move(p);
			} break;
			case 'o':
				if (!output.empty())
					CAGE_THROW_ERROR(exception, "output name specified multiple times");
				output = cmd->argument();
				break;
			}
		}
		if (channels == 0)
			CAGE_THROW_ERROR(exception, "no inputs specified");
		if (output.empty())
			output = "material.png";

		CAGE_LOG(severityEnum::Info, "material-tool", string() + "combining image");
		holder<pngImageClass> res = newPngImage();
		res->empty(width, height, channels);
		for (uint32 i = 0; i < channels; i++)
		{
			holder<pngImageClass> &src = pngs[i];
			if (!src)
				continue;
			for (uint32 y = 0; y < height; y++)
			{
				for (uint32 x = 0; x < width; x++)
					res->value(x, y, i, src->value(x, y, 0));
			}
		}

		CAGE_LOG(severityEnum::Info, "material-tool", string() + "saving image");
		res->encodeFile(output);

		CAGE_LOG(severityEnum::Info, "material-tool", string() + "ok");
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
