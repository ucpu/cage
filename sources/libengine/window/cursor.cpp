#include "private.h"

#include <cage-core/image.h>
#include <cage-engine/cursor.h>

namespace cage
{
	namespace
	{
		class CursorImpl : public Cursor
		{
		public:
			GLFWcursor *data = nullptr;

			CursorImpl(const CursorCreateConfig &config)
			{
				CAGE_ASSERT(config.image);
				if (config.image->channels() != 4 || config.image->format() != ImageFormatEnum::U8)
					CAGE_THROW_ERROR(Exception, "image is incompatible for use as cursor");

				GLFWimage g = {};
				g.width = config.image->width();
				g.height = config.image->height();
				g.pixels = (unsigned char *)config.image->rawViewU8().data();

				data = glfwCreateCursor(&g, config.hotSpot[0], config.hotSpot[1]);

				if (!data)
					CAGE_THROW_ERROR(Exception, "failed to create cursor");
			}

			~CursorImpl()
			{
				if (data)
					glfwDestroyCursor(data);
			}
		};
	}

	Holder<Cursor> newCursor(const CursorCreateConfig &config)
	{
		return systemMemory().createImpl<Cursor, CursorImpl>(config);
	}

	namespace privat
	{
		GLFWcursor *getCursor(Cursor *c)
		{
			const CursorImpl *impl = (const CursorImpl *)c;
			return impl->data;
		}
	}
}
