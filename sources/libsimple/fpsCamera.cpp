#include <cage-core/entities.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/scene.h>
#include <cage-engine/window.h>
#include <cage-simple/engine.h>
#include <cage-simple/fpsCamera.h>

namespace cage
{
	namespace
	{
		class FpsCameraImpl : public FpsCamera
		{
		public:
			const EventListener<bool(const GenericInput &)> windowListener = engineWindow()->events.listen([this](const GenericInput &in) { return this->event(in); });
			const EventListener<bool()> updateListener = controlThread().update.listen([this]() { return this->update(); });
			VariableSmoothingBuffer<Vec2, 1> mouseSmoother;
			VariableSmoothingBuffer<Vec3, 3> moveSmoother;
			VariableSmoothingBuffer<Real, 2> wheelSmoother;
			Vec2 mouseMoveAccum;
			Real wheelAccum;

			Entity *ent = nullptr;
			bool keysPressedArrows[10] = {}; // wsadeq up down left right

			explicit FpsCameraImpl(Entity *ent) : ent(ent) {}

			bool mouseEnabled(MouseButtonsFlags buttons) { return !!ent && engineWindow()->isFocused() && (mouseButton == MouseButtonsFlags::None || any(buttons & mouseButton)); }

			bool mousePress(input::MousePress in)
			{
				if (mouseEnabled(in.buttons))
					engineWindow()->mouseRelativeMovement(true);
				return false;
			}

			bool mouseRelativeMove(input::MouseRelativeMove in)
			{
				if (!mouseEnabled(in.buttons))
					return false;
				mouseMoveAccum -= in.position;
				return false;
			}

			bool mouseWheel(input::MouseWheel in)
			{
				if (!ent)
					return false;
				wheelAccum += in.wheel;
				return false;
			}

			bool setKey(uint32 k, bool v)
			{
				switch (k)
				{
					case 87: // w
						keysPressedArrows[0] = v;
						return true;
					case 83: // s
						keysPressedArrows[1] = v;
						return true;
					case 65: // a
						keysPressedArrows[2] = v;
						return true;
					case 68: // d
						keysPressedArrows[3] = v;
						return true;
					case 69: // e
						keysPressedArrows[4] = v;
						return true;
					case 81: // q
						keysPressedArrows[5] = v;
						return true;
					case 265: // up
						keysPressedArrows[6] = v;
						return true;
					case 264: // down
						keysPressedArrows[7] = v;
						return true;
					case 263: // left
						keysPressedArrows[8] = v;
						return true;
					case 262: // right
						keysPressedArrows[9] = v;
						return true;
				}
				return false;
			}

			bool keyPress(input::KeyPress in) { return setKey(in.key, true); }

			bool keyRelease(input::KeyRelease in)
			{
				setKey(in.key, false);
				return false;
			}

			bool event(GenericInput in)
			{
				if (in.has<input::MousePress>())
					return mousePress(in.get<input::MousePress>());
				if (in.has<input::MouseRelativeMove>())
					return mouseRelativeMove(in.get<input::MouseRelativeMove>());
				if (in.has<input::MouseWheel>())
					return mouseWheel(in.get<input::MouseWheel>());
				if (in.has<input::KeyPress>())
					return keyPress(in.get<input::KeyPress>());
				if (in.has<input::KeyRelease>())
					return keyRelease(in.get<input::KeyRelease>());
				return false;
			}

			void update()
			{
				if (!ent)
					return;

				if (!mouseEnabled(engineWindow()->mouseButtons()))
					engineWindow()->mouseRelativeMovement(false);

				TransformComponent &t = ent->value<TransformComponent>();

				// orientation
				mouseSmoother.add(mouseMoveAccum);
				mouseMoveAccum = Vec2();
				wheelSmoother.add(wheelAccum);
				wheelAccum = 0;
				const Vec2 r = mouseSmoother.smooth() * turningSpeed;
				if (freeMove)
					t.orientation = t.orientation * Quat(Rads(r[1]), Rads(r[0]), Degs(wheelSmoother.smooth() * rollSpeed));
				else
				{ // limit pitch
					Vec3 f = t.orientation * Vec3(0, 0, -1);
					Rads pitch = asin(f[1]);
					f[1] = 0;
					f = normalize(f);
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
					f[1] = 0;
					f = normalize(f);
					l[1] = 0;
					l = normalize(l);
				}
				Vec3 move1;
				if (keysPressedArrows[0] && keysWsadEnabled) // w
					move1 += f;
				if (keysPressedArrows[1] && keysWsadEnabled) // s
					move1 -= f;
				if (keysPressedArrows[2] && keysWsadEnabled) // a
					move1 += l;
				if (keysPressedArrows[3] && keysWsadEnabled) // d
					move1 -= l;
				if (keysPressedArrows[4] && keysEqEnabled) // e
					move1 += u;
				if (keysPressedArrows[5] && keysEqEnabled) // q
					move1 -= u;
				if (keysPressedArrows[6] && keysArrowsEnabled) // up
					move1 += f;
				if (keysPressedArrows[7] && keysArrowsEnabled) // down
					move1 -= f;
				if (keysPressedArrows[8] && keysArrowsEnabled) // left
					move1 += l;
				if (keysPressedArrows[9] && keysArrowsEnabled) // right
					move1 -= l;
				if (lengthSquared(move1) > 1e-5)
					move1 = normalize(move1) * movementSpeed;
				else
					move1 = Vec3();
				moveSmoother.add(move1);
				t.position += moveSmoother.smooth();
			}
		};
	}

	void FpsCamera::setEntity(Entity *ent)
	{
		FpsCameraImpl *impl = (FpsCameraImpl *)this;
		impl->ent = ent;
	}

	Holder<FpsCamera> newFpsCamera(Entity *ent)
	{
		return systemMemory().createImpl<FpsCamera, FpsCameraImpl>(ent);
	}
}
