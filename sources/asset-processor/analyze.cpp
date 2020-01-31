#include "processor.h"
#include <cage-core/debug.h>

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
