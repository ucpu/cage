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
			cfg.rootPath = inputDirectory;
			cfg.passInvalidVectors = true;
			MeshImportResult result = meshImportFiles(inputFileName, cfg);
			meshImportConvertToCageFormats(result);
			writeLine("cage-begin");
			try
			{
				// models
				if (result.parts.size() == 1)
				{
					writeLine("scheme=model");
					writeLine(Stringizer() + "asset=" + inputFile);
				}
				else
					for (const auto &part : result.parts)
					{
						writeLine("scheme=model");
						writeLine(Stringizer() + "asset=" + inputFile + "?" + part.objectName + "_" + part.materialName);
					}

				// textures
				for (const auto &part : result.parts)
				{
					for (const auto &tex : part.textures)
					{
						writeLine("scheme=texture");
						CAGE_ASSERT(isPattern(tex.name, inputDirectory, "", ""));
						writeLine(Stringizer() + "asset=" + subString(tex.name, inputDirectory.length() + 1, m));
					}
				}

				// skeletons
				if (result.skeleton)
				{
					writeLine("scheme=skeleton");
					writeLine(Stringizer() + "asset=" + inputFile + ";skeleton");
				}

				// animations
				for (const auto &ani : result.animations)
				{
					writeLine("scheme=animation");
					writeLine(Stringizer() + "asset=" + inputFile + "?" + ani.name);
				}
			}
			catch (...)
			{}
			writeLine("cage-end");
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
	writeLine("cage-stop");
	return 0;
}
