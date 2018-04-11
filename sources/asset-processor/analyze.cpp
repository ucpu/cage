#include "processor.h"

int processAnalyze()
{
	detail::overrideBreakpoint overrideBreakpoint;
	analyzeTexture();
	analyzeAssimp();
	analyzeFont();
	analyzeSound();
	writeLine("cage-stop");
	return 0;
}
