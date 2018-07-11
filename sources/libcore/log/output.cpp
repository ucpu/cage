#include <cstdio>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/filesystem.h>

namespace cage
{
	void logOutputPolicyDebug(const string &message)
	{
		detail::debugOutput(message.c_str());
	}

	void logOutputPolicyStdOut(const string &message)
	{
		fprintf(stdout, "%s\n", message.c_str());
		fflush(stdout);
	}

	void logOutputPolicyStdErr(const string &message)
	{
		fprintf(stderr, "%s\n", message.c_str());
		fflush(stderr);
	}

	namespace
	{
		class fileOutputPolicyImpl : public logOutputPolicyFileClass
		{
		public:
			fileOutputPolicyImpl(const string &path, bool append)
			{
				f = newFile(path, fileMode(false, true, true, append));
			}

			holder<fileClass> f;
		};
	}

	void logOutputPolicyFileClass::output(const string &message)
	{
		fileOutputPolicyImpl *impl = (fileOutputPolicyImpl*)this;
		impl->f->writeLine(message);
		impl->f->flush();
	}

	holder<logOutputPolicyFileClass> newLogOutputPolicyFile(const string &path, bool append)
	{
		return detail::systemArena().createImpl<logOutputPolicyFileClass, fileOutputPolicyImpl>(path, append);
	}
}
