#include "processor.h"

#include <cage-core/debug.h>
#include <cage-core/meshImport.h>

namespace
{
	void analyzeAssimp()
	{
		try
		{
			MeshImportConfig cfg;
			cfg.rootPath = processor->inputDirectory;
			cfg.passInvalidVectors = true;
			MeshImportResult result = meshImportFiles(processor->inputFileName, cfg);
			meshImportConvertToCageFormats(result);
			processor->writeLine("cage-begin");
			try
			{
				// models
				if (result.parts.size() == 1)
				{
					processor->writeLine("scheme=model");
					processor->writeLine(Stringizer() + "asset=" + processor->inputFile);
				}
				else
					for (const auto &part : result.parts)
					{
						processor->writeLine("scheme=model");
						processor->writeLine(Stringizer() + "asset=" + processor->inputFile + "?" + part.objectName + "_" + part.materialName);
					}

				// textures
				for (const auto &part : result.parts)
				{
					for (const auto &tex : part.textures)
					{
						processor->writeLine("scheme=texture");
						CAGE_ASSERT(isPattern(tex.name, processor->inputDirectory, "", ""));
						processor->writeLine(Stringizer() + "asset=" + subString(tex.name, processor->inputDirectory.length() + 1, m));
					}
				}

				// skeletons
				if (result.skeleton)
				{
					processor->writeLine("scheme=skeleton");
					processor->writeLine(Stringizer() + "asset=" + processor->inputFile + ";skeleton");
				}

				// animations
				for (const auto &ani : result.animations)
				{
					processor->writeLine("scheme=animation");
					processor->writeLine(Stringizer() + "asset=" + processor->inputFile + "?" + ani.name);
				}
			}
			catch (...)
			{}
			processor->writeLine("cage-end");
		}
		catch (...)
		{
			// do nothing
		}
	}
}

int processAnalyze()
{
	detail::OverrideBreakpoint OverrideBreakpoint;
	analyzeTexture();
	analyzeAssimp();
	analyzeFont();
	analyzeSound();
	processor->writeLine("cage-stop");
	return 0;
}
