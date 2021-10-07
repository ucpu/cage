#include <cage-core/debug.h>
#include <cage-core/meshImport.h>

#include "processor.h"

namespace
{
	void analyzeAssimp()
	{
		try
		{
			const MeshImportResult result = meshImportFiles(inputFileName);
			writeLine("cage-begin");
			try
			{
				// models
				if (result.parts.size() == 1)
				{
					writeLine("scheme=model");
					writeLine(Stringizer() + "asset=" + inputFile);
				}
				else for (const auto &part : result.parts)
				{
					writeLine("scheme=model");
					writeLine(Stringizer() + "asset=" + inputFile + "?" + part.objectName + "_" + part.materialName);
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
			{
			}
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
