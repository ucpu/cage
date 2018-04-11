namespace cage
{
	void graphicPrepareCreate(const engineCreateConfig &config);
	void graphicPrepareDestroy();
	void graphicPrepareInitialize();
	void graphicPrepareFinalize();
	void graphicPrepareEmit();
	void graphicPrepareTick(uint64 controlTime, uint64 prepareTime);

	void graphicDispatchCreate(const engineCreateConfig &config);
	void graphicDispatchDestroy();
	void graphicDispatchInitialize();
	void graphicDispatchFinalize();
	void graphicDispatchTick();
	void graphicDispatchSwap();

	void soundCreate(const engineCreateConfig &config);
	void soundDestroy();
	void soundInitialize();
	void soundFinalize();
	void soundEmit();
	void soundTick(uint64 controlTime, uint64 prepareTime);
}
