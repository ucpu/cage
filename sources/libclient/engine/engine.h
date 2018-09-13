namespace cage
{
	void graphicsPrepareCreate(const engineCreateConfig &config);
	void graphicsPrepareDestroy();
	void graphicsPrepareEmit(uint64 time);
	void graphicsPrepareTick(uint64 time);

	void graphicsDispatchCreate(const engineCreateConfig &config);
	void graphicsDispatchDestroy();
	void graphicsDispatchInitialize();
	void graphicsDispatchFinalize();
	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives);
	void graphicsDispatchSwap();

	void soundCreate(const engineCreateConfig &config);
	void soundDestroy();
	void soundFinalize();
	void soundEmit(uint64 time);
	void soundTick(uint64 time);
}
