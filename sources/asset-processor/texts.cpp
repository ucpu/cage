#include "processor.h"

#include <cage-core/texts.h>
#include <cage-core/unicode.h>

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

	enum class MsgType
	{
		None,
		Id,
		Str,
	};

	void parsePO(Holder<File> f, const LanguageCode &lang, Texts *txt)
	{
		String id, str, l;
		MsgType type = MsgType::None;

		const auto &quotes = [](String s) -> String
		{
			s = trim(s, true, true, "\t ");
			if (s.length() >= 2 && s[0] == '\"' && s[s.length() - 1] == '\"')
				s = subString(s, 1, s.length() - 2);
			s = replace(s, "\\r\\n", "\n");
			s = replace(s, "\\n\\r", "\n");
			s = replace(s, "\\r", "\n");
			s = replace(s, "\\n", "\n");
			s = replace(s, "\\t", "\t");
			s = replace(s, "\\\"", "\"");
			return s;
		};

		const auto &add = [&]()
		{
			if (!id.empty() && !str.empty())
				txt->set(id, str, lang);
			id = str = "";
			type = MsgType::None;
		};

		while (f->readLine(l))
		{
			l = unicodeTransformString(l, { UnicodeTransformEnum::CanonicalComposition });
			l = trim(l);
			if (l.empty())
			{
				add();
				continue;
			}
			if (l[0] == '#')
				continue;

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
				id = quotes(l);
				type = MsgType::Id;
			}
			else if (k == "msgstr")
			{
				if (!str.empty())
				{
					CAGE_LOG(SeverityEnum::Note, "assetProcessor", id);
					CAGE_LOG(SeverityEnum::Note, "assetProcessor", p);
					CAGE_THROW_ERROR(Exception, "multiple msgstr");
				}
				str = quotes(l);
				type = MsgType::Str;
			}
			else
			{
				switch (type)
				{
					case MsgType::Id:
						id += quotes(p);
						break;
					case MsgType::Str:
						str += quotes(p);
						break;
					default:
						CAGE_LOG(SeverityEnum::Note, "assetProcessor", p);
						CAGE_THROW_ERROR(Exception, "unknown command");
						break;
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
		for (const LanguageCode &n : l)
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
