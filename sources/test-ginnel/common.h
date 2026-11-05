#include <cage-core/networkGinnel.h>

using namespace cage;

class Conn
{
public:
	bool process();
};

Holder<Conn> newConn(Holder<GinnelConnection> udp);

struct Runner
{
	Runner();
	void step();

private:
	Time last = {};
	Time period = {};
};
