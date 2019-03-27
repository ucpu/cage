#include "filesystem.h"

namespace cage
{
	void pathCreateArchive(const string &path, const string &type)
	{
		CAGE_THROW_ERROR(notImplementedException, "pathCreateArchive");
	}

	void archiveMove(std::shared_ptr<archiveVirtual> &af, const string &pf, std::shared_ptr<archiveVirtual> &at, const string &pt)
	{
		CAGE_THROW_ERROR(notImplementedException, "archiveMove");
	}

	std::shared_ptr<archiveVirtual> archiveTryOpen(const string &path, string &insidePath)
	{
		return {};
	}
}

