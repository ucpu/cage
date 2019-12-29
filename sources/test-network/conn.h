
class connClass
{
public:
	bool process();
};

Holder<connClass> newConn(Holder<UdpConnection> udp);
