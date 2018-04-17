#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include <cage-client/engine.h>
#include <cage-client/utility/cameraController.h>

namespace cage
{
	namespace
	{
		class cameraControllerImpl : public cameraControllerClass
		{
		public:
			windowEventListeners listeners;
			eventListener<bool()> updateListener;

			entityClass *entity;
			bool keysPressedArrows[6]; // wsadeq

			cameraControllerImpl(entityClass *entity) : entity(entity)
			{
				for (uint32 i = 0; i < 6; i++)
					keysPressedArrows[i] = false;
				movementSpeed = 1;
				turningSpeed = vec2(0.008, 0.008);
				pitchLimitUp = degs(50);
				pitchLimitDown = degs(-50);
				mouseButton = (mouseButtonsFlags)0;
				keysEqEnabled = true;
				keysWsadEnabled = true;
				keysArrowsEnabled = true;
				freeMove = false;

				listeners.mousePress.bind<cameraControllerImpl, &cameraControllerImpl::mousePress>(this);
				listeners.mouseMove.bind<cameraControllerImpl, &cameraControllerImpl::mouseMove>(this);
				listeners.keyPress.bind<cameraControllerImpl, &cameraControllerImpl::keyPress>(this);
				listeners.keyRelease.bind<cameraControllerImpl, &cameraControllerImpl::keyRelease>(this);
				listeners.attachAll(window());
				updateListener.bind<cameraControllerImpl, &cameraControllerImpl::update>(this);
				controlThread::update.attach(updateListener);
			}

			const pointStruct centerMouse()
			{
				auto w = window();
				pointStruct pt2 = w->resolution();
				pt2.x /= 2;
				pt2.y /= 2;
				w->mousePosition(pt2);
				return pt2;
			}

			bool mouseEnabled(mouseButtonsFlags buttons)
			{
				return window()->isFocused() && (mouseButton == mouseButtonsFlags::None || (buttons & mouseButton) == mouseButton);
			}

			bool mousePress(mouseButtonsFlags buttons, modifiersFlags, const pointStruct &)
			{
				if (mouseEnabled(buttons))
					centerMouse();
				return false;
			}

			bool mouseMove(mouseButtonsFlags buttons, modifiersFlags, const pointStruct &pt)
			{
				if (!entity)
					return false;
				if (!mouseEnabled(buttons))
					return false;
				pointStruct pt2 = centerMouse();
				sint32 dx = pt2.x - pt.x;
				sint32 dy = pt2.y - pt.y;
				if (abs(dx) + abs(dy) > 100)
					return false;
				vec2 r = vec2(dx, dy) * turningSpeed;
				ENGINE_GET_COMPONENT(transform, t, entity);
				t.orientation = t.orientation * quat(rads(r[1]), rads(r[0]), degs());
				if (!freeMove)
				{ // limit pitch
					vec3 f = t.orientation * vec3(0, 0, -1);
					rads pitch = aSin(f[1]);
					f[1] = 0; f = f.normalize();
					rads yaw = aTan2(-f[0], f[2]) + degs(90);
					pitch = clamp(pitch, pitchLimitDown, pitchLimitUp);
					t.orientation = quat(pitch, yaw, degs());
				}
				return false;
			}

			bool setKey(uint32 k, bool v)
			{
				switch (k)
				{
				case 87: // w
					if (keysWsadEnabled)
						keysPressedArrows[0] = v;
					return true;
				case 265: // up
					if (keysArrowsEnabled)
						keysPressedArrows[0] = v;
					return true;
				case 83: // s
					if (keysWsadEnabled)
						keysPressedArrows[1] = v;
					return true;
				case 264: // down
					if (keysArrowsEnabled)
						keysPressedArrows[1] = v;
					return true;
				case 65: // a
					if (keysWsadEnabled)
						keysPressedArrows[2] = v;
					return true;
				case 263: // left
					if (keysArrowsEnabled)
						keysPressedArrows[2] = v;
					return true;
				case 68: // d
					if (keysWsadEnabled)
						keysPressedArrows[3] = v;
					return true;
				case 262: // right
					if (keysArrowsEnabled)
						keysPressedArrows[3] = v;
					return true;
				case 69: // e
					if (keysEqEnabled)
						keysPressedArrows[4] = v;
					return true;
				case 81: // q
					if (keysEqEnabled)
						keysPressedArrows[5] = v;
					return true;
				}
				return false;
			}

			bool keyPress(uint32 a, uint32, modifiersFlags)
			{
				return setKey(a, true);
			}

			bool keyRelease(uint32 a, uint32, modifiersFlags)
			{
				return setKey(a, false);
			}

			bool update()
			{
				if (!entity)
					return false;
				ENGINE_GET_COMPONENT(transform, t, entity);
				vec3 f = t.orientation * vec3(0, 0, -1);
				vec3 l = t.orientation * vec3(-1, 0, 0);
				vec3 u = vec3(0, 1, 0);
				if (freeMove)
					u = t.orientation * u;
				else
				{
					f[1] = 0; f = f.normalize();
					l[1] = 0; l = l.normalize();
				}
				vec3 movement = vec3();
				if (keysPressedArrows[0]) movement += f;
				if (keysPressedArrows[1]) movement -= f;
				if (keysPressedArrows[2]) movement += l;
				if (keysPressedArrows[3]) movement -= l;
				if (keysPressedArrows[4]) movement += u;
				if (keysPressedArrows[5]) movement -= u;
				if (movement != vec3())
					t.position += movement.normalize() * movementSpeed;
				return false;
			}
		};
	}

	void cameraControllerClass::setEntity(entityClass *entity)
	{
		cameraControllerImpl *impl = (cameraControllerImpl*)this;
		impl->entity = entity;
	}

	holder<cameraControllerClass> newCameraController(entityClass *entity)
	{
		return detail::systemArena().createImpl<cameraControllerClass, cameraControllerImpl>(entity);
	}
}
