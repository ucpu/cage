#include <vector>

#include "database.h"

#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/process.h>
#include <cage-core/tasks.h>

namespace cage
{
	void AssetsDatabaseImpl::checkOutput()
	{
		const PathTypeFlags t = pathType(config.outputPath);
		if (config.outputArchive && any(t & PathTypeFlags::NotFound))
			return pathCreateArchiveCarch(config.outputPath);
		if (any(t & PathTypeFlags::Archive))
			return;
		// the output is not an archive, output to it directly
		config.intermediatePath = "";
	}

	namespace
	{
		struct Processor
		{
			const AssetsDatabaseImpl *impl = nullptr;
			std::vector<DatabaseAssetImpl *> asses;

			void processAsset(DatabaseAssetImpl &ass) const
			{
				CAGE_LOG(SeverityEnum::Info, "asset", ass.name);
				ass.corrupted = true;
				detail::OverrideBreakpoint overrideBreakpoint;
				try
				{
					ass.files.clear();
					ass.references.clear();

					const auto &scheme = impl->schemes.at(ass.scheme);
					if (!scheme->applyOnAsset(ass))
						CAGE_THROW_ERROR(Exception, "asset has invalid configuration");

					Holder<Process> prg = newProcess(ProcessCreateConfig(scheme->processor, impl->config.processorWorkingDir));
					prg->writeLine(Stringizer() + "inputDirectory=" + pathToAbs(impl->config.inputPath)); // inputDirectory
					prg->writeLine(Stringizer() + "inputName=" + ass.name); // inputName
					prg->writeLine(Stringizer() + "outputDirectory=" + pathToAbs(impl->config.intermediatePath.empty() ? impl->config.outputPath : impl->config.intermediatePath)); // outputDirectory
					prg->writeLine(Stringizer() + "outputName=" + ass.id); // outputName
					prg->writeLine(Stringizer() + "schemeIndex=" + scheme->schemeIndex); // schemeIndex
					for (const auto &it : ass.fields)
						prg->writeLine(Stringizer() + it.first + "=" + it.second);
					prg->writeLine("cage-end");

					bool begin = false, end = false;
					while (true)
					{
						String line = prg->readLine();
						if (line.empty() || line == "cage-error")
						{
							CAGE_THROW_ERROR(Exception, "processing thrown an error");
						}
						else if (line == "cage-begin")
						{
							if (end || begin)
								CAGE_THROW_ERROR(Exception, "unexpected cage-begin");
							begin = true;
						}
						else if (line == "cage-end")
						{
							if (end || !begin)
								CAGE_THROW_ERROR(Exception, "unexpected cage-end");
							end = true;
							break;
						}
						else if (begin && !end)
						{
							const String param = trim(split(line, "="));
							line = trim(line);
							if (param == "use")
							{
								if (pathIsAbs(line))
								{
									CAGE_LOG_THROW(Stringizer() + "path: " + line);
									CAGE_THROW_ERROR(Exception, "assets use path must be relative");
								}
								if (!pathIsFile(pathJoin(pathToAbs(impl->config.inputPath), line)))
								{
									CAGE_LOG_THROW(Stringizer() + "path: " + line);
									CAGE_THROW_ERROR(Exception, "assets use path does not exist");
								}
								ass.files.insert(line);
							}
							else if (param == "ref")
								ass.references.insert(line);
							else
							{
								CAGE_LOG_THROW(Stringizer() + "parameter: " + param + ", value: " + line);
								CAGE_THROW_ERROR(Exception, "unknown parameter name");
							}
						}
						else
							CAGE_LOG(SeverityEnum::Note, "processor", line);
					}

					const sint32 ret = prg->wait();
					if (ret != 0)
						CAGE_THROW_ERROR(SystemError, "process returned error code", ret);

					if (ass.files.empty())
						CAGE_THROW_ERROR(Exception, "asset reported no used files");

					ass.corrupted = false;
					ass.modified = true;
				}
				catch (const Exception &e)
				{
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "failed processing asset: " + ass.name);
				}
				catch (...)
				{
					detail::logCurrentCaughtException();
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unknown exception while processing asset: " + ass.name);
				}
			}

			void operator()(uint32 index) const
			{
				CAGE_ASSERT(index < asses.size());
				processAsset(*asses[index]);
			}
		};
	}

	void AssetsDatabaseImpl::processAssets()
	{
		if (!config.intermediatePath.empty())
			pathRemove(config.intermediatePath);

		{
			Processor processor;
			processor.impl = this;
			processor.asses.reserve(assets.size());
			for (const auto &it : assets)
				if (it.second->corrupted)
					processor.asses.push_back(+it.second);
			tasksRunBlocking("processing", processor, processor.asses.size());
		}

		updateStatus();

		if (!config.intermediatePath.empty() && pathIsDirectory(config.intermediatePath))
			pathMove(config.intermediatePath, config.outputPath);
	}
}
