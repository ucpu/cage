
class connClass
{
public:
	bool process();
};

holder<connClass> newConn(holder<udpConnection> udp);
