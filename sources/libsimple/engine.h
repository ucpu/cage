#ifndef header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu
#define header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu

#include <cage-simple/engine.h>

namespace cage
{
	void graphicsCreate(const EngineCreateConfig &config);
	void graphicsDestroy();
	void graphicsInitialize(); // opengl thread
	void graphicsFinalize(); // opengl thread
	void graphicsEmit(uint64 time); // control thread
	void graphicsPrepare(uint64 time, uint32 &drawCalls, uint32 &drawPrimitives); // prepare thread
	void graphicsDispatch(); // opengl thread
	void graphicsSwap(); // opengl thread

	void soundCreate(const EngineCreateConfig &config);
	void soundDestroy();
	void soundFinalize();
	void soundEmit(uint64 time);
	void soundTick(uint64 time);

	extern EntityComponent *transformHistoryComponent;
}

#endif
