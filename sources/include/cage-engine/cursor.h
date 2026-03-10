#ifndef guard_cursor_h_d5tuz74jz85uihopfgztu
#define guard_cursor_h_d5tuz74jz85uihopfgztu

#include <cage-engine/core.h>

namespace cage
{
	class Image;

	class CAGE_ENGINE_API Cursor : private Immovable
	{};

	struct CAGE_ENGINE_API CursorCreateConfig
	{
		const Image *image = nullptr;
		Vec2i hotSpot;
	};

	CAGE_ENGINE_API Holder<Cursor> newCursor(const CursorCreateConfig &config);
}

#endif
