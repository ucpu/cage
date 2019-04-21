namespace cage
{
	struct CAGE_API discoveryPeerStruct
	{
		string message;
		string address;
		uint16 port;

		discoveryPeerStruct() : port(0)
		{}
	};

	class CAGE_API discoveryClientClass
	{
	public:
		void update();
		void addServer(const string &address, uint16 port);
		uint32 peersCount() const;
		discoveryPeerStruct peerData(uint32 index) const;
		holder<pointerRange<discoveryPeerStruct>> peers() const;
	};

	class CAGE_API discoveryServerClass
	{
	public:
		void update();
		string message;
	};

	CAGE_API holder<discoveryClientClass> newDiscoveryClient(uint16 listenPort, uint32 gameId);
	CAGE_API holder<discoveryServerClass> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId);
}
