#include <steam/isteamnetworkingsockets.h>

namespace cage
{
	struct Init
	{
		Init() { SteamNetworkingSockets(); }
	} init;
}
