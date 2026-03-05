#include <cage-core/audio.h>
#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

using namespace cage;

void convert(String src, const String &format, bool preserveOriginal)
{
	src = pathSimplify(src);
	const String path = pathExtractDirectory(src);
	const String dst = pathJoin(path, pathExtractFilenameNoExtension(src) + format);
	CAGE_LOG(SeverityEnum::Info, "soundConvert", Stringizer() + "converting: " + src + ", to " + dst);
	if (src == dst)
	{
		CAGE_LOG(SeverityEnum::Info, "soundConvert", "no conversion required");
		return;
	}
	Holder<Audio> snd = newAudio();
	snd->importFile(src);
	CAGE_LOG(SeverityEnum::Info, "soundConvert", Stringizer() + "channels: " + snd->channels() + ", sample rate: " + snd->sampleRate() + ", frames: " + snd->frames());
	snd->exportFile(dst);
	if (!preserveOriginal)
		pathRemove(src);
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		Holder<Ini> cmd = newIni();
		cmd->parseCmd(argc, args);
		const auto paths = cmd->cmdArray(0, "--");
		const bool preserveOriginal = cmd->cmdBool('p', "preserve", false);
		const String format = cmd->cmdString('f', "format", ".png");
		if (cmd->cmdBool('?', "help", false))
		{
			cmd->logHelp();
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "example:");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + args[0] + " --preserve --format .ogg -- a.wav b.flac");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "  to convert a.wav to a.ogg and b.flac to b.ogg");
			CAGE_LOG(SeverityEnum::Info, "help", Stringizer() + "--preserve: keep original files too");
			return 0;
		}
		cmd->checkUnusedWithHelp();
		if (paths.empty())
			CAGE_THROW_ERROR(Exception, "no inputs");
		for (const String &path : paths)
			convert(path, format, preserveOriginal);
		CAGE_LOG(SeverityEnum::Info, "soundConvert", "done");
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
