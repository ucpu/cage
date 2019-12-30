#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/variableSmoothingBuffer.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include <cage-engine/engine.h>
#include <cage-engine/fpsCamera.h>

namespace cage
{
	namespace
	{
		class FpsCameraImpl : public FpsCamera
		{
		public:
			WindowEventListeners listeners;
			EventListener<void()> updateListener;
			VariableSmoothingBuffer<vec2, 1> mouseSmoother;
			VariableSmoothingBuffer<vec3, 3> moveSmoother;
			VariableSmoothingBuffer<real, 2> wheelSmoother;
			vec2 mouseMoveAccum;
			real wheelAccum;

			Entity *ent;
			bool keysPressedArrows[6]; // wsadeq

			FpsCameraImpl(Entity *ent) : ent(ent)
			{
				for (uint32 i = 0; i < 6; i++)
					keysPressedArrows[i] = false;

				listeners.mousePress.bind<FpsCameraImpl, &FpsCameraImpl::mousePress>(this);
				listeners.mouseMove.bind<FpsCameraImpl, &FpsCameraImpl::mouseMove>(this);
				listeners.mouseWheel.bind<FpsCameraImpl, &FpsCameraImpl::mouseWheel>(this);
				listeners.keyPress.bind<FpsCameraImpl, &FpsCameraImpl::keyPress>(this);
				listeners.keyRelease.bind<FpsCameraImpl, &FpsCameraImpl::keyRelease>(this);
				listeners.attachAll(window());
				updateListener.bind<FpsCameraImpl, &FpsCameraImpl::update>(this);
				controlThread().update.attach(updateListener);
			}

			const ivec2 centerMouse()
			{
				auto w = window();
				ivec2 pt2 = w->resolution();
				pt2.x /= 2;
				pt2.y /= 2;
				w->mousePosition(pt2);
				return pt2;
			}

			bool mouseEnabled(MouseButtonsFlags buttons)
			{
				return !!ent && window()->isFocused() && (mouseButton == MouseButtonsFlags::None || (buttons & mouseButton) == mouseButton);
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags, const ivec2 &)
			{
				if (mouseEnabled(buttons))
					centerMouse();
				return false;
			}

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags, const ivec2 &pt)
			{
				if (!mouseEnabled(buttons))
					return false;
				ivec2 pt2 = centerMouse();
				sint32 dx = pt2.x - pt.x;
				sint32 dy = pt2.y - pt.y;
				mouseMoveAccum += vec2(dx, dy);
				return false;
			}

			bool mouseWheel(sint32 wheel, ModifiersFlags, const ivec2 &)
			{
				if (!ent)
					return false;
				wheelAccum += wheel;
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

			bool keyPress(uint32 a, uint32, ModifiersFlags)
			{
				return setKey(a, true);
			}

			bool keyRelease(uint32 a, uint32, ModifiersFlags)
			{
				return setKey(a, false);
			}

			void update()
			{
				if (!ent)
					return;
				CAGE_COMPONENT_ENGINE(Transform, t, ent);

				// orientation
				mouseSmoother.add(mouseMoveAccum);
				mouseMoveAccum = vec2();
				wheelSmoother.add(wheelAccum);
				wheelAccum = 0;
				vec2 r = mouseSmoother.smooth() * turningSpeed;
				if (freeMove)
				{
					t.orientation = t.orientation * quat(rads(r[1]), rads(r[0]), degs(wheelSmoother.smooth() * wheelSpeed));
				}
				else
				{ // limit pitch
					vec3 f = t.orientation * vec3(0, 0, -1);
					rads pitch = asin(f[1]);
					f[1] = 0; f = normalize(f);
					rads yaw = atan2(-f[0], f[2]) + degs(90) + rads(r[0]);
					pitch = clamp(pitch + rads(r[1]), pitchLimitDown, pitchLimitUp);
					t.orientation = quat(pitch, yaw, degs());
				}

				// movement
				vec3 f = t.orientation * vec3(0, 0, -1);
				vec3 l = t.orientation * vec3(-1, 0, 0);
				vec3 u = vec3(0, 1, 0);
				if (freeMove)
					u = t.orientation * u;
				else
				{
					f[1] = 0; f = normalize(f);
					l[1] = 0; l = normalize(l);
				}
				vec3 movement = vec3();
				if (keysPressedArrows[0]) movement += f;
				if (keysPressedArrows[1]) movement -= f;
				if (keysPressedArrows[2]) movement += l;
				if (keysPressedArrows[3]) movement -= l;
				if (keysPressedArrows[4]) movement += u;
				if (keysPressedArrows[5]) movement -= u;
				if (movement != vec3())
					moveSmoother.add(normalize(movement) * movementSpeed);
				else
					moveSmoother.add(vec3());
				t.position += moveSmoother.smooth();
			}
		};
	}

	FpsCamera::FpsCamera() : movementSpeed(1), wheelSpeed(10), turningSpeed(0.008), pitchLimitUp(degs(80)), pitchLimitDown(degs(-80)), mouseButton(MouseButtonsFlags::None), keysEqEnabled(true), keysWsadEnabled(true), keysArrowsEnabled(true), freeMove(false)
	{}

	void FpsCamera::setEntity(Entity *ent)
	{
		FpsCameraImpl *impl = (FpsCameraImpl*)this;
		impl->ent = ent;
	}

	Holder<FpsCamera> newFpsCamera(Entity *ent)
	{
		return detail::systemArena().createImpl<FpsCamera, FpsCameraImpl>(ent);
	}
}
