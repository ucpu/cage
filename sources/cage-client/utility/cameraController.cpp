#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/engine.h>
#include <cage-client/utility/cameraController.h>

namespace cage
{
	namespace
	{
		class cameraControllerImpl : public cameraControllerClass
		{
		public:
			eventListener<bool(windowClass *w, mouseButtonsFlags, modifiersFlags, const pointStruct &pt)> mouseMoveListener;
			eventListener<bool(windowClass *w, mouseButtonsFlags, modifiersFlags, const pointStruct &pt)> mousePressListener;
			eventListener<bool(windowClass *w, uint32 a, uint32 b, modifiersFlags m)> keyPressListener;
			eventListener<bool(windowClass *w, uint32 a, uint32 b, modifiersFlags m)> keyReleaseListener;
			eventListener<bool(uint64)> updateListener;

			vec2 mouseMoveCurrent;
			vec2 mouseMoveLast;
			entityClass *entity;
			volatile bool keysPressedArrows[6]; // wsadeq

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

				mouseMoveListener.bind<cameraControllerImpl, &cameraControllerImpl::mouseMove>(this);
				mousePressListener.bind<cameraControllerImpl, &cameraControllerImpl::mousePress>(this);
				keyPressListener.bind<cameraControllerImpl, &cameraControllerImpl::keyPress>(this);
				keyReleaseListener.bind<cameraControllerImpl, &cameraControllerImpl::keyRelease>(this);
				updateListener.bind<cameraControllerImpl, &cameraControllerImpl::update>(this);

				window()->events.mouseMove.add(mouseMoveListener);
				window()->events.mousePress.add(mousePressListener);
				window()->events.keyPress.add(keyPressListener);
				window()->events.keyRelease.add(keyReleaseListener);
				controlThread::update.add(updateListener);
			}

			const pointStruct centerMouse(windowClass *w)
			{
				pointStruct pt2 = w->resolution();
				pt2.x /= 2;
				pt2.y /= 2;
				w->mousePosition(pt2);
				return pt2;
			}

			bool mouseEnabled(windowClass *w, mouseButtonsFlags buttons)
			{
				return w->isFocused() && (mouseButton == mouseButtonsFlags::None || (buttons & mouseButton) == mouseButton);
			}

			bool mousePress(windowClass *w, mouseButtonsFlags buttons, modifiersFlags, const pointStruct &pt)
			{
				if (mouseEnabled(w, buttons))
					centerMouse(w);
				return false;
			}

			bool mouseMove(windowClass *w, mouseButtonsFlags buttons, modifiersFlags, const pointStruct &pt)
			{
				if (mouseEnabled(w, buttons))
				{
					pointStruct pt2 = centerMouse(w);
					sint32 dx = pt.x - pt2.x;
					sint32 dy = pt.y - pt2.y;
					if (abs(dx) + abs(dy) < 100)
						mouseMoveCurrent += vec2(dx, dy);
				}
				return false;
			}

			void setKey(uint32 k, bool v)
			{
				switch (k)
				{
				case 87: // w
					if (keysWsadEnabled)
						keysPressedArrows[0] = v;
					break;
				case 265: // up
					if (keysArrowsEnabled)
						keysPressedArrows[0] = v;
					break;
				case 83: // s
					if (keysWsadEnabled)
						keysPressedArrows[1] = v;
					break;
				case 264: // down
					if (keysArrowsEnabled)
						keysPressedArrows[1] = v;
					break;
				case 65: // a
					if (keysWsadEnabled)
						keysPressedArrows[2] = v;
					break;
				case 263: // left
					if (keysArrowsEnabled)
						keysPressedArrows[2] = v;
					break;
				case 68: // d
					if (keysWsadEnabled)
						keysPressedArrows[3] = v;
					break;
				case 262: // right
					if (keysArrowsEnabled)
						keysPressedArrows[3] = v;
					break;
				case 69: // e
					if (keysEqEnabled)
						keysPressedArrows[4] = v;
					break;
				case 81: // q
					if (keysEqEnabled)
						keysPressedArrows[5] = v;
					break;
				}
			}

			bool keyPress(windowClass *w, uint32 a, uint32 b, modifiersFlags m)
			{
				setKey(a, true);
				return false;
			}

			bool keyRelease(windowClass *w, uint32 a, uint32 b, modifiersFlags m)
			{
				setKey(a, false);
				return false;
			}

			bool update(uint64 time)
			{
				if (!entity)
					return false;
				ENGINE_GET_COMPONENT(transform, t, entity);
				{ // turning
					vec2 c = mouseMoveCurrent;
					vec2 r = (c - mouseMoveLast) * -turningSpeed;
					mouseMoveLast = c;
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
				}
				{ // movement
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
				}
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