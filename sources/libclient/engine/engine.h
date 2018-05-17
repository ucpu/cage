namespace cage
{
	void graphicsPrepareCreate(const engineCreateConfig &config);
	void graphicsPrepareDestroy();
	void graphicsPrepareInitialize();
	void graphicsPrepareFinalize();
	void graphicsPrepareEmit();
	void graphicsPrepareTick(uint64 controlTime, uint64 prepareTime);

	void graphicsDispatchCreate(const engineCreateConfig &config);
	void graphicsDispatchDestroy();
	void graphicsDispatchInitialize();
	void graphicsDispatchFinalize();
	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives);
	void graphicsDispatchSwap();

	void soundCreate(const engineCreateConfig &config);
	void soundDestroy();
	void soundInitialize();
	void soundFinalize();
	void soundEmit();
	void soundTick(uint64 controlTime, uint64 prepareTime);
}
