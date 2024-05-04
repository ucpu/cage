#include "processor.h"

#include <cage-core/texts.h>

namespace
{
	LanguageCode extractLanguage(const String &pth)
	{
		// eg. pth = dir/gui.en_US.po
		const String n = pathExtractFilenameNoExtension(pth); // -> gui.en_US
		const String l = subString(pathExtractExtension(n), 1, m); // -> en_US
		// eg. inputFile = dir/gui.pot
		const String p = pathExtractFilenameNoExtension(inputFile); // -> gui
		if (p + "." + l == n)
			return l;
		return "";
	}

	void parsePO(Holder<File> f, const LanguageCode &lang, Texts *txt)
	{
		String id, str, l;

		const auto &quotes = [](const String &s) -> String
		{
			if (s.length() >= 2 && s[0] == '\"' && s[s.length() - 1] == '\"')
				return subString(s, 1, s.length() - 2);
			return s;
		};

		const auto &add = [&]()
		{
			if (!id.empty() && !str.empty())
				txt->set(quotes(id), quotes(str), lang);
			else if (!id.empty() || !str.empty())
			{
				CAGE_LOG(SeverityEnum::Note, "assetProcessor", id);
				CAGE_LOG(SeverityEnum::Note, "assetProcessor", str);
				CAGE_THROW_ERROR(Exception, "missing msgstr or msgid");
			}
			id = str = "";
		};

		while (f->readLine(l))
		{
			l = trim(l);
			if (l.empty())
				add();
			else if (l[0] != '#')
			{
				const String p = l;
				String k = split(l, "\t ");
				k = toLower(k);
				if (k == "msgid")
				{
					if (!id.empty())
					{
						CAGE_LOG(SeverityEnum::Note, "assetProcessor", id);
						CAGE_LOG(SeverityEnum::Note, "assetProcessor", p);
						CAGE_THROW_ERROR(Exception, "multiple msgid");
					}
					id = l;
				}
				else if (k == "msgstr")
				{
					if (!str.empty())
					{
						CAGE_LOG(SeverityEnum::Note, "assetProcessor", id);
						CAGE_LOG(SeverityEnum::Note, "assetProcessor", p);
						CAGE_THROW_ERROR(Exception, "multiple msgstr");
					}
					str = l;
				}
				else
				{
					CAGE_LOG(SeverityEnum::Note, "assetProcessor", p);
					CAGE_THROW_ERROR(Exception, "unknown command");
				}
			}
		}
		add();
	}
}

void processTexts()
{
	writeLine(String("use=") + inputFile);

	{
		const String ext = pathExtractExtension(inputFile);
		if (ext != ".pot" && ext != ".po")
			CAGE_THROW_ERROR(Exception, "input file must have .pot or .po extension");
	}

	Holder<Texts> txt = newTexts();

	if (toBool(properties("multilingual")))
	{
		{ // validate template file
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "validating template");
			Holder<Texts> tmp = newTexts();
			parsePO(readFile(inputFileName), "", +tmp);
		}

		for (const String &pth : pathListDirectory(pathExtractDirectory(inputFileName)))
		{
			if (!pathIsFile(pth))
				continue;
			if (pathExtractExtension(pth) != ".po")
				continue;
			const LanguageCode lang = extractLanguage(pth);
			if (lang.empty())
				continue;
			writeLine(String("use=") + pathToRel(pth, inputDirectory));
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "loading language: " + lang);
			parsePO(readFile(pth), lang, +txt);
		}
	}
	else
	{
		parsePO(readFile(inputFileName), "", +txt);
	}

	{
		const auto l = txt->allLanguages();
		if (l.empty())
			CAGE_THROW_ERROR(Exception, "loaded no languages");
		for (const String &n : l)
			CAGE_LOG(SeverityEnum::Info, "language", n);
	}
	{
		const auto l = txt->allNames();
		if (l.empty())
			CAGE_THROW_ERROR(Exception, "loaded no texts");
		for (const String &n : l)
			CAGE_LOG(SeverityEnum::Info, "name", n);
	}

	Holder<PointerRange<char>> buff = txt->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
