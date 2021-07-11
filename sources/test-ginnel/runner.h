
struct Runner
{
	Runner();
	void step();

private:
	uint64 last = 0;
	uint64 period = 0;
};

