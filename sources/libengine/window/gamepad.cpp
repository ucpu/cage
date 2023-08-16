#include <map>
#include <vector>

#include "private.h"

#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/flatSet.h>
#include <cage-core/serialization.h>
#include <cage-engine/gamepad.h>

namespace cage
{
	namespace
	{
		struct GamepadsData
		{
			FlatSet<int> available;
			std::map<int, class GamepadImpl *> used;
			Holder<Mutex> mut = newMutex();
		};

		GamepadsData &gamepadsData()
		{
			static GamepadsData data;
			return data;
		}

		class GamepadImpl : public Gamepad
		{
		public:
			String name;
			ConcurrentQueue<GenericInput> eventsQueue;
			std::vector<Real> axes1, axes2;
			std::vector<char> buts1, buts2;
			int jid = m;
			bool connected = true;
			bool mapped = false;

			GamepadImpl()
			{
				{
					ScopeLock l(cageGlfwMutex());
					glfwPollEvents();
				}
				GamepadsData &d = gamepadsData();
				ScopeLock l(d.mut);
				if (d.available.empty())
					CAGE_THROW_ERROR(Exception, "no new gamepad available");
				jid = *d.available.begin();
				CAGE_ASSERT(glfwJoystickPresent(jid));
				d.available.erase(jid);
				CAGE_ASSERT(d.used.count(jid) == 0);
				d.used[jid] = this;
				name = glfwGetJoystickName(jid);
				if (!glfwJoystickIsGamepad(jid))
					CAGE_LOG(SeverityEnum::Warning, "gamepad", Stringizer() + "gamepad '" + name + "' does not have input mapping");
				processEventsImpl();
			}

			~GamepadImpl()
			{
				GamepadsData &d = gamepadsData();
				ScopeLock l(d.mut);
				d.used.erase(jid);
				if (glfwJoystickPresent(jid))
					d.available.insert(jid);
			}

			void processEventsImpl()
			{
				if (GLFWgamepadstate state; glfwGetGamepadState(jid, &state))
				{
					mapped = true;
					static_assert(sizeof(GLFWgamepadstate::axes[0]) == sizeof(axes1[0]));
					axes1.resize(sizeof(GLFWgamepadstate::axes) / sizeof(GLFWgamepadstate::axes[0]));
					detail::memcpy(axes1.data(), state.axes, sizeof(state.axes));
					static_assert(sizeof(GLFWgamepadstate::buttons[0]) == sizeof(buts1[0]));
					buts1.resize(sizeof(GLFWgamepadstate::buttons) / sizeof(GLFWgamepadstate::buttons[0]));
					detail::memcpy(buts1.data(), state.buttons, sizeof(state.buttons));
				}
				else
				{
					mapped = false;
					int size = 0;
					const float *as = glfwGetJoystickAxes(jid, &size);
					axes1.resize(size);
					detail::memcpy(axes1.data(), as, size * sizeof(float));
					size = 0;
					const unsigned char *bs = glfwGetJoystickButtons(jid, &size);
					buts1.resize(size);
					detail::memcpy(buts1.data(), bs, size * sizeof(char));
				}

				axes2.resize(axes1.size());
				buts2.resize(buts1.size());

				for (uint32 i = 0; i < axes1.size(); i++)
				{
					Real &a = axes1[i];
					if (abs(a) < deadzone)
						a = 0;
					if (a != axes2[i])
						eventsQueue.push(GenericInput{ InputGamepadAxis{ this, i, axes1[i] }, InputClassEnum::GamepadAxis });
				}

				for (uint32 i = 0; i < buts1.size(); i++)
				{
					if (buts1[i] && !buts2[i])
						eventsQueue.push(GenericInput{ InputGamepadKey{ this, i }, InputClassEnum::GamepadPress });
					else if (!buts1[i] && buts2[i])
						eventsQueue.push(GenericInput{ InputGamepadKey{ this, i }, InputClassEnum::GamepadRelease });
				}

				axes2 = axes1;
				buts2 = buts1;
			}

			void processEvents()
			{
				{
					ScopeLock l(cageGlfwMutex());
					glfwPollEvents();
				}
				processEventsImpl();
				{
					GenericInput e;
					while (eventsQueue.tryPop(e))
						events.dispatch(e);
				}
			}
		};

		void gamepadCallback(int jid, int event)
		{
			GamepadsData &d = gamepadsData();
			ScopeLock l(d.mut);
			switch (event)
			{
				case GLFW_CONNECTED:
				{
					if (d.used.count(jid))
					{
						GamepadImpl *g = d.used[jid];
						g->connected = true;
						g->eventsQueue.push(GenericInput{ InputGamepadState{ g }, InputClassEnum::GamepadConnected });
					}
					else
						d.available.insert(jid);
				}
				break;
				case GLFW_DISCONNECTED:
				{
					if (d.used.count(jid))
					{
						GamepadImpl *g = d.used[jid];
						g->connected = false;
						g->eventsQueue.push(GenericInput{ InputGamepadState{ g }, InputClassEnum::GamepadDisconnected });
					}
					else
						d.available.erase(jid);
				}
				break;
			}
		}
	}

	void cageGlfwInitializeGamepads()
	{
		glfwSetJoystickCallback(&gamepadCallback);
		GamepadsData &d = gamepadsData();
		for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++)
		{
			if (glfwJoystickPresent(i))
				d.available.insert(i);
		}
	}

	void Gamepad::processEvents()
	{
		GamepadImpl *impl = (GamepadImpl *)this;
		impl->processEvents();
	}

	String Gamepad::name() const
	{
		GamepadImpl *impl = (GamepadImpl *)this;
		return impl->name;
	}

	bool Gamepad::connected() const
	{
		const GamepadImpl *impl = (const GamepadImpl *)this;
		return impl->connected;
	}

	bool Gamepad::mapped() const
	{
		const GamepadImpl *impl = (const GamepadImpl *)this;
		return impl->mapped;
	}

	PointerRange<const Real> Gamepad::axes() const
	{
		GamepadImpl *impl = (GamepadImpl *)this;
		return impl->axes2;
	}

	PointerRange<const bool> Gamepad::buttons() const
	{
		GamepadImpl *impl = (GamepadImpl *)this;
		return bufferCast<const bool, const char>(impl->buts2);
	}

	Holder<Gamepad> newGamepad()
	{
		cageGlfwInitializeFunc();
		return systemMemory().createImpl<Gamepad, GamepadImpl>();
	}

	uint32 gamepadsAvailable()
	{
		cageGlfwInitializeFunc();
		{
			ScopeLock l(cageGlfwMutex());
			glfwPollEvents();
		}
		GamepadsData &d = gamepadsData();
		ScopeLock l(d.mut);
		return numeric_cast<uint32>(d.available.size());
	}
}
