#include <cstring>
#include <cmath>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/utility/cmdOptions.h>

namespace cage
{
	namespace
	{
		class cmdOptionsImpl : public cmdOptionsClass
		{
		private:
			const char **m_argv;
			const char *m_pszOptions;
			const char *m_pszArg;
			uint32 m_argc;
			uint32 m_nCurr;

		public:
			cmdOptionsImpl(uint32 argc, const char *args[], const char *options) : m_argv(args), m_pszOptions(options), m_pszArg(0), m_argc(argc), m_nCurr(1) {}

			bool next()
			{
				m_pszArg = 0;
				if (m_nCurr >= m_argc)
				{
					return 0;
				}

				if ((m_argv[m_nCurr][0] != '-' && m_argv[m_nCurr][0] != '/') || m_argv[m_nCurr][1] == 0 || m_argv[m_nCurr][1] == ':' || m_argv[m_nCurr][2] != 0)
				{
					CAGE_LOG(severityEnum::Note, "exception", string(m_argv[m_nCurr]));
					CAGE_THROW_ERROR(exception, "invalid input");
				}

				m_pszArg = strchr(m_pszOptions, m_argv[m_nCurr][1]);
				if (!m_pszArg)
				{
					CAGE_LOG(severityEnum::Note, "exception", string(m_argv[m_nCurr]));
					CAGE_THROW_ERROR(exception, "invalid input");
				}

				if (*(m_pszArg + 1) == ':')
				{
					if (m_nCurr + 1 >= m_argc)
					{
						m_pszArg = 0;
						CAGE_LOG(severityEnum::Note, "exception", string(m_argv[m_nCurr]));
						CAGE_THROW_ERROR(exception, "invalid input");
					}
					m_nCurr += 2;
				}
				else
				{
					++m_nCurr;
				}

				return *m_pszArg != 0;
			}

			char option() const
			{
				return m_pszArg ? *m_pszArg : (char)0;
			}

			const char *argument() const
			{
				if (*(m_pszArg + 1) != ':' || m_nCurr <= 1)
				{
					CAGE_THROW_ERROR(exception, "invalid input");
				}
				return m_argv[m_nCurr - 1];
			}
		};
	}

	bool cmdOptionsClass::next()
	{
		cmdOptionsImpl *impl = (cmdOptionsImpl*)this;
		return impl->next();
	}

	char cmdOptionsClass::option() const
	{
		cmdOptionsImpl *impl = (cmdOptionsImpl*)this;
		return impl->option();
	}

	const char *cmdOptionsClass::argument() const
	{
		cmdOptionsImpl *impl = (cmdOptionsImpl*)this;
		return impl->argument();
	}

	holder<cmdOptionsClass> newCmdOptions(const uint32 argc, const char *args[], const char *options)
	{
		return detail::systemArena().createImpl <cmdOptionsClass, cmdOptionsImpl>(argc, args, options);
	}
}