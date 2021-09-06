#include <cage-core/entities.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/window.h>
#include <cage-engine/scene.h>

#include <cage-simple/engine.h>
#include <cage-simple/fpsCamera.h>

namespace cage
{
	namespace
	{
		class FpsCameraImpl : public FpsCamera
		{
		public:
			WindowEventListeners listeners;
			EventListener<void()> updateListener;
			VariableSmoothingBuffer<Vec2, 1> mouseSmoother;
			VariableSmoothingBuffer<Vec3, 3> moveSmoother;
			VariableSmoothingBuffer<Real, 2> wheelSmoother;
			Vec2 mouseMoveAccum;
			Real wheelAccum;

			Entity *ent = nullptr;
			bool keysPressedArrows[6] = {false, false, false, false, false, false}; // wsadeq

			FpsCameraImpl(Entity *ent) : ent(ent)
			{
				listeners.mousePress.bind<FpsCameraImpl, &FpsCameraImpl::mousePress>(this);
				listeners.mouseMove.bind<FpsCameraImpl, &FpsCameraImpl::mouseMove>(this);
				listeners.mouseWheel.bind<FpsCameraImpl, &FpsCameraImpl::mouseWheel>(this);
				listeners.keyPress.bind<FpsCameraImpl, &FpsCameraImpl::keyPress>(this);
				listeners.keyRelease.bind<FpsCameraImpl, &FpsCameraImpl::keyRelease>(this);
				listeners.attachAll(engineWindow());
				updateListener.bind<FpsCameraImpl, &FpsCameraImpl::update>(this);
				controlThread().update.attach(updateListener);
			}

			const Vec2i centerMouse()
			{
				auto w = engineWindow();
				Vec2i pt2 = w->resolution();
				pt2[0] /= 2;
				pt2[1] /= 2;
				w->mousePosition(pt2);
				return pt2;
			}

			bool mouseEnabled(MouseButtonsFlags buttons)
			{
				return !!ent && engineWindow()->isFocused() && (mouseButton == MouseButtonsFlags::None || (buttons & mouseButton) == mouseButton);
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags, const Vec2i &)
			{
				if (mouseEnabled(buttons))
					centerMouse();
				return false;
			}

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags, const Vec2i &pt)
			{
				if (!mouseEnabled(buttons))
					return false;
				Vec2i pt2 = centerMouse();
				mouseMoveAccum += Vec2(pt2 - pt);
				return false;
			}

			bool mouseWheel(sint32 wheel, ModifiersFlags, const Vec2i &)
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

			bool keyPress(uint32 key, ModifiersFlags)
			{
				return setKey(key, true);
			}

			bool keyRelease(uint32 key, ModifiersFlags)
			{
				return setKey(key, false);
			}

			void update()
			{
				if (!ent)
					return;
				TransformComponent &t = ent->value<TransformComponent>();

				// orientation
				mouseSmoother.add(mouseMoveAccum);
				mouseMoveAccum = Vec2();
				wheelSmoother.add(wheelAccum);
				wheelAccum = 0;
				Vec2 r = mouseSmoother.smooth() * turningSpeed;
				if (freeMove)
				{
					t.orientation = t.orientation * Quat(Rads(r[1]), Rads(r[0]), Degs(wheelSmoother.smooth() * rollSpeed));
				}
				else
				{ // limit pitch
					Vec3 f = t.orientation * Vec3(0, 0, -1);
					Rads pitch = asin(f[1]);
					f[1] = 0; f = normalize(f);
					Rads yaw = atan2(-f[0], f[2]) + Degs(90) + Rads(r[0]);
					pitch = clamp(pitch + Rads(r[1]), pitchLimitDown, pitchLimitUp);
					t.orientation = Quat(pitch, yaw, Degs());
				}

				// movement
				Vec3 f = t.orientation * Vec3(0, 0, -1);
				Vec3 l = t.orientation * Vec3(-1, 0, 0);
				Vec3 u = Vec3(0, 1, 0);
				if (freeMove)
					u = t.orientation * u;
				else
				{
					f[1] = 0; f = normalize(f);
					l[1] = 0; l = normalize(l);
				}
				Vec3 movement = Vec3();
				if (keysPressedArrows[0]) movement += f;
				if (keysPressedArrows[1]) movement -= f;
				if (keysPressedArrows[2]) movement += l;
				if (keysPressedArrows[3]) movement -= l;
				if (keysPressedArrows[4]) movement += u;
				if (keysPressedArrows[5]) movement -= u;
				if (movement != Vec3())
					moveSmoother.add(normalize(movement) * movementSpeed);
				else
					moveSmoother.add(Vec3());
				t.position += moveSmoother.smooth();
			}
		};
	}

	void FpsCamera::setEntity(Entity *ent)
	{
		FpsCameraImpl *impl = (FpsCameraImpl*)this;
		impl->ent = ent;
	}

	Holder<FpsCamera> newFpsCamera(Entity *ent)
	{
		return systemMemory().createImpl<FpsCamera, FpsCameraImpl>(ent);
	}
}
