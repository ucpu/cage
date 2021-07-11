
class Conn
{
public:
	bool process();
};

Holder<Conn> newConn(Holder<GinnelConnection> udp);
