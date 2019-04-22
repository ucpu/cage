
class connClass
{
public:
	bool process();
};

holder<connClass> newConn(holder<udpConnectionClass> udp);

static const uint64 timeStep = 1000000 / 30;
