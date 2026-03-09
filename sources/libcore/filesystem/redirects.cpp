#include <algorithm>
#include <vector>

#include "files.h"

#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
		std::vector<std::pair<String, String>> &mapping()
		{
			static std::vector<std::pair<String, String>> *data = new std::vector<std::pair<String, String>>; // intentional leak
			return *data;
		}

		auto find(const String &p)
		{
			auto &mp = mapping();
			for (auto it = mp.begin(), et = mp.end(); it != et; it++)
				if (it->first == p)
					return it;
			return mp.end();
		}

		bool isPathPrefix(const String &path, const String &prefix)
		{
			if (isPattern(path, prefix, "", ""))
				return path.length() == prefix.length() || path[prefix.length()] == '/';
			return false;
		}
	}

	Holder<PointerRange<String>> fixupDirectoryListingRedirections(Holder<PointerRange<String>> &&list, const String &originalPath)
	{
		if (mapping().empty())
			return list;

		const String abs = pathToAbs(originalPath);
		PointerRangeHolder<String> res;
		res.reserve(list.size());

		// rename found entries with original path
		for (const String &l : list)
			res.push_back(pathJoin(abs, pathExtractFilename(l)));

		// append redirection entries in the same folder
		for (const auto &it : mapping())
		{
			if (pathJoin(it.first, "..") == abs)
				res.push_back(it.first);
		}

		// deduplicate
		std::sort(res.begin(), res.end());
		res.erase(std::unique(res.begin(), res.end()), res.end());

		return res;
	}

	void pathCreateRedirect(const String &virtualPath, const String &redirectsTo)
	{
		ScopeLock lock(fsMutex());
		const String src = pathToAbs(virtualPath);
		const String dst = pathToAbs(redirectsTo);
		if (src == dst)
			return;
		CAGE_LOG(SeverityEnum::Info, "fileRedirects", Stringizer() + "creating file redirect from: " + src + ", to: " + dst);
		if (find(src) != mapping().end())
			CAGE_THROW_ERROR(Exception, "redirect already exists");
		if (isPathPrefix(src, dst) || isPathPrefix(dst, src))
			CAGE_THROW_ERROR(Exception, "cannot create file redirect to its prefix");
		pathJoin(src, ".."); // make sure that the path is not root
		{
			const auto t = pathType(src);
			if (any(t) && none(t & PathTypeFlags::NotFound))
				CAGE_LOG(SeverityEnum::Warning, "fileRedirects", "file redirect will hide already existing path");
		}
		CAGE_ASSERT(find(src) == mapping().end());
		mapping().push_back({ src, dst });
	}

	void pathRemoveRedirect(const String &virtualPath)
	{
		ScopeLock lock(fsMutex());
		const String src = pathToAbs(virtualPath);
		auto it = find(src);
		if (it == mapping().end())
			return;
		CAGE_LOG(SeverityEnum::Info, "fileRedirects", Stringizer() + "removing file redirect from: " + src + ", to: " + it->second);
		mapping().erase(it);
	}

	String pathResolveRedirects(const String &path)
	{
		ScopeLock lock(fsMutex());
		String src = pathToAbs(path);
		for (const auto &it : mapping())
		{
			if (isPathPrefix(src, it.first))
			{
				src = it.second + remove(src, 0, it.first.length());
				return pathResolveRedirects(src); // must check additional redirects
			}
		}
		return path; // original path (not abs) if no redirects
	}

	Holder<PointerRange<std::pair<String, String>>> pathListRedirects()
	{
		ScopeLock lock(fsMutex());
		PointerRangeHolder<std::pair<String, String>> res;
		res.reserve(mapping().size());
		for (const auto &it : mapping())
			res.push_back({ it.first, it.second });
		return res;
	}
}
