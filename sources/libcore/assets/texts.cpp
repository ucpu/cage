#include <vector>

#include <unordered_dense.h>

#include <cage-core/assetContext.h>
#include <cage-core/concurrent.h>
#include <cage-core/containerSerialization.h>
#include <cage-core/flatSet.h>
#include <cage-core/hashString.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>
#include <cage-core/texts.h>

namespace cage
{
	namespace
	{
		struct LanguageValues : private Noncopyable
		{
			LanguageCode lang;
			ankerl::unordered_dense::map<uint32, String> values;
		};

		Serializer &operator<<(Serializer &s, const LanguageValues &v)
		{
			s << v.lang << v.values;
			return s;
		}

		Deserializer &operator>>(Deserializer &s, LanguageValues &v)
		{
			s >> v.lang >> v.values;
			return s;
		}

		class TextsImpl : public Texts
		{
		public:
			std::vector<LanguageValues> values;
			FlatSet<LanguageCode> languages;
			ankerl::unordered_dense::set<String> names;
			FlatSet<uint32> ids;

			void clear()
			{
				values.clear();
				languages.clear();
				names.clear();
				ids.clear();
			}

			void importBuffer(PointerRange<const char> buffer)
			{
				clear();
				Deserializer des(buffer);
				des >> values >> languages >> names >> ids;
				CAGE_ASSERT(des.available() == 0);
			}

			Holder<PointerRange<char>> exportBuffer() const
			{
				MemoryBuffer buff;
				Serializer ser(buff);
				ser << values << languages << names << ids;
				return std::move(buff);
			}

			ankerl::unordered_dense::map<uint32, String> &map(const LanguageCode &lang)
			{
				for (auto &it : values)
				{
					if (it.lang == lang)
						return it.values;
				}
				values.push_back({});
				values.back().lang = lang;
				languages.insert(lang);
				return values.back().values;
			}
		};
	}

	void Texts::clear()
	{
		TextsImpl *impl = (TextsImpl *)this;
		impl->clear();
	}

	Holder<Texts> Texts::copy() const
	{
		Holder<Texts> res = newTexts();
		res->importBuffer(exportBuffer()); // todo more efficient
		return res;
	}

	Holder<PointerRange<char>> Texts::exportBuffer() const
	{
		const TextsImpl *impl = (const TextsImpl *)this;
		return impl->exportBuffer();
	}

	void Texts::importBuffer(PointerRange<const char> buffer)
	{
		TextsImpl *impl = (TextsImpl *)this;
		impl->importBuffer(buffer);
	}

	void Texts::set(uint32 id, const String &text, const LanguageCode &language)
	{
		CAGE_ASSERT(id != 0);
		TextsImpl *impl = (TextsImpl *)this;
		impl->map(language)[id] = text;
		impl->ids.insert(id);
	}

	uint32 Texts::set(const String &name, const String &text, const LanguageCode &language)
	{
		CAGE_ASSERT(name != "");
		TextsImpl *impl = (TextsImpl *)this;
		const uint32 id = HashString(name);
		set(id, text, language);
		impl->names.insert(name);
		return id;
	}

	Holder<PointerRange<LanguageCode>> Texts::allLanguages() const
	{
		const TextsImpl *impl = (const TextsImpl *)this;
		return PointerRangeHolder<LanguageCode>(impl->languages.begin(), impl->languages.end());
	}

	Holder<PointerRange<String>> Texts::allNames() const
	{
		const TextsImpl *impl = (const TextsImpl *)this;
		return PointerRangeHolder<String>(impl->names.begin(), impl->names.end());
	}

	Holder<PointerRange<uint32>> Texts::allIds() const
	{
		const TextsImpl *impl = (const TextsImpl *)this;
		return PointerRangeHolder<uint32>(impl->ids.begin(), impl->ids.end());
	}

	String Texts::get(uint32 id, const LanguageCode &language) const
	{
		CAGE_ASSERT(id != 0);
		const TextsImpl *impl = (const TextsImpl *)this;
		for (const auto &it : impl->values)
		{
			if (it.lang == language)
			{
				auto it2 = it.values.find(id);
				if (it2 != it.values.end())
					return it2->second;
				break;
			}
		}
		return "";
	}

	Holder<Texts> newTexts()
	{
		return systemMemory().createImpl<Texts, TextsImpl>();
	}

	namespace
	{
		struct TextsAsset
		{
			Holder<Texts> t;

			~TextsAsset()
			{
				if (t)
					textsRemove(+t);
			}
		};

		void processTextsLoad(AssetContext *context)
		{
			Holder<Texts> h = newTexts();
			h->importBuffer(context->originalData);
			Holder<TextsAsset> ass = systemMemory().createHolder<TextsAsset>();
			ass->t = std::move(h);
			textsAdd(+ass->t);
			Holder<Texts> t = Holder<Texts>(+ass->t, std::move(ass));
			context->assetHolder = std::move(t).cast<void>();
		}
	}

	AssetScheme genAssetSchemeTexts()
	{
		AssetScheme s;
		s.load.bind<&processTextsLoad>();
		s.typeHash = detail::typeHash<Texts>();
		return s;
	}

	namespace
	{
		Holder<RwMutex> mut = newRwMutex();
		std::vector<LanguageCode> languages = { "en_US", "" };
		FlatSet<const Texts *> sources;
	}

	void textsSetLanguages(PointerRange<const LanguageCode> langs)
	{
		ScopeLock lock(mut, WriteLockTag());
		languages = std::vector<LanguageCode>(langs.begin(), langs.end());
	}

	void textsSetLanguages(const String &languages)
	{
		std::vector<LanguageCode> v;
		String l = languages;
		while (!l.empty())
			v.push_back(split(l, ";"));
		textsSetLanguages(v);
	}

	void textsAdd(const Texts *txt)
	{
		ScopeLock lock(mut, WriteLockTag());
		sources.insert(txt);
	}

	void textsRemove(const Texts *txt)
	{
		ScopeLock lock(mut, WriteLockTag());
		sources.erase(txt);
	}

	String textsGet(uint32 id, String params)
	{
		if (id == 0)
			return params;
		ScopeLock lock(mut, ReadLockTag());
		for (const LanguageCode &lang : languages)
		{
			for (const auto src : sources)
			{
				const String s = src->get(id, lang);
				if (!s.empty())
					return textFormat(s, params);
			}
		}
		return params;
	}

	String textFormat(String res, const String &params)
	{
		uint32 pc = 1;
		for (char c : params)
			pc += c == '|';

		const auto &find = [&](uint32 idx) -> String
		{
			if (idx > pc)
				return "";
			uint32 i = 0;
			uint32 k = 0;
			while (k < idx)
				k += params[i++] == '|';
			String res;
			while (i < params.length() && params[i] != '|')
				res += String(params[i++]);
			return res;
		};

		while (true)
		{
			String prev = split(res, "{");
			if (res == "")
				return prev + res;
			String mid = split(res, "}");
			if (!mid.empty() && isDigitsOnly(mid))
			{
				const uint32 idx = toUint32(mid);
				mid = find(idx);
			}
			else
				mid = "";
			res = prev + mid + res;
		}
		return res;
	}
}
