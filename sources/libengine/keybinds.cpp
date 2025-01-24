#include <algorithm>
#include <variant>
#include <vector>

#include <cage-engine/keybinds.h>

namespace cage
{
	namespace
	{
		std::vector<class KeybindImpl *> global;

		enum class MatchResultEnum
		{
			Unmatched = 0,
			Matched,
			Activate,
			Deactivate,
		};

		CAGE_FORCE_INLINE MatchResultEnum operator||(MatchResultEnum a, MatchResultEnum b)
		{
			if (a != MatchResultEnum::Unmatched)
				return a;
			return b;
		}

		struct MatcherBase
		{
			ModifiersFlags requiredFlags = ModifiersFlags::None;
			ModifiersFlags forbiddenFlags = ModifiersFlags::None;

			CAGE_FORCE_INLINE bool checkFlags(ModifiersFlags mods) const
			{
				if (any(mods & forbiddenFlags))
					return false;
				return (mods & requiredFlags) == requiredFlags;
			}
		};

		struct KeyboardMatcher : public MatcherBase
		{
			uint32 key = 0;

			CAGE_FORCE_INLINE MatchResultEnum matchImpl(input::privat::BaseKey k, MatchResultEnum activation) const
			{
				if (k.key == key && checkFlags(k.mods))
					return activation;
				return MatchResultEnum::Unmatched;
			}

			template<class T>
			CAGE_FORCE_INLINE MatchResultEnum match(const GenericInput &input, MatchResultEnum activation) const
			{
				if (input.has<T>())
					return matchImpl(input.get<T>(), activation);
				return MatchResultEnum::Unmatched;
			}

			CAGE_FORCE_INLINE MatchResultEnum match(const GenericInput &input) const { return match<input::KeyPress>(input, MatchResultEnum::Activate) || match<input::KeyRepeat>(input, MatchResultEnum::Activate) || match<input::KeyRelease>(input, MatchResultEnum::Deactivate); }
		};

		struct MouseMatcher : public MatcherBase
		{
			MouseButtonsFlags button = MouseButtonsFlags::None;

			CAGE_FORCE_INLINE MatchResultEnum matchImpl(input::privat::BaseMouse k, MatchResultEnum activation) const
			{
				if (any(k.buttons & button) && checkFlags(k.mods))
					return activation;
				return MatchResultEnum::Unmatched;
			}

			template<class T>
			CAGE_FORCE_INLINE MatchResultEnum match(const GenericInput &input, MatchResultEnum activation) const
			{
				if (input.has<T>())
					return matchImpl(input.get<T>(), activation);
				return MatchResultEnum::Unmatched;
			}

			CAGE_FORCE_INLINE MatchResultEnum match(const GenericInput &input) const { return match<input::MousePress>(input, MatchResultEnum::Activate) || match<input::MouseDoublePress>(input, MatchResultEnum::Activate) || match<input::MouseRelease>(input, MatchResultEnum::Deactivate); }
		};

		struct WheelMatcher : public MatcherBase
		{
			CAGE_FORCE_INLINE MatchResultEnum match(const GenericInput &input) const
			{
				if (input.has<input::MouseWheel>())
				{
					if (checkFlags(input.get<input::MouseWheel>().mods))
						return MatchResultEnum::Matched;
				}
				return MatchResultEnum::Unmatched;
			}
		};

		using Matcher = std::variant<std::monostate, KeyboardMatcher, MouseMatcher, WheelMatcher>;

		CAGE_FORCE_INLINE MatchResultEnum matches(const GenericInput &input, const Matcher &matcher)
		{
			return std::visit(
				[input](const auto &mt) -> MatchResultEnum
				{
					if constexpr (std::is_same_v<std::decay_t<decltype(mt)>, std::monostate>)
						return MatchResultEnum::Unmatched;
					else
						return mt.match(input);
				},
				matcher);
		}

		struct Maker
		{
			const KeybindCreateConfig &config;
			Matcher result;

			CAGE_FORCE_INLINE MatcherBase base(ModifiersFlags mods) const
			{
				MatcherBase r;
				r.requiredFlags = config.requiredFlags | mods;
				r.forbiddenFlags = config.forbiddenFlags;
				CAGE_ASSERT(none(r.requiredFlags & r.forbiddenFlags));
				return r;
			}

			CAGE_FORCE_INLINE void makeImpl(input::privat::BaseKey k)
			{
				if (none(config.devices & KeybindDevicesFlags::Keyboard))
					return;
				if (any(k.mods & config.forbiddenFlags))
					return;
				result = KeyboardMatcher{ base(k.mods), k.key };
			}

			CAGE_FORCE_INLINE void makeImpl(input::privat::BaseMouse k)
			{
				if (none(config.devices & KeybindDevicesFlags::Mouse))
					return;
				if (any(k.mods & config.forbiddenFlags))
					return;
				result = MouseMatcher{ base(k.mods), k.buttons };
			}

			CAGE_FORCE_INLINE void makeImpl(input::MouseWheel k)
			{
				if (none(config.devices & KeybindDevicesFlags::Wheel))
					return;
				if (any(k.mods & config.forbiddenFlags))
					return;
				result = WheelMatcher{ base(k.mods) };
			}

			template<class T>
			CAGE_FORCE_INLINE void make(const GenericInput &input)
			{
				if (!std::holds_alternative<std::monostate>(result))
					return;
				if (input.has<T>())
					makeImpl(input.get<T>());
			}

			void make(const GenericInput &input)
			{
				make<input::KeyPress>(input);
				make<input::KeyRepeat>(input);
				make<input::KeyRelease>(input);
				make<input::MousePress>(input);
				make<input::MouseDoublePress>(input);
				make<input::MouseRelease>(input);
				make<input::MouseWheel>(input);
			}
		};

		Matcher makeMatcher(const KeybindCreateConfig &config, const GenericInput &input)
		{
			Maker maker{ config };
			maker.make(input);
			if (!std::holds_alternative<std::monostate>(maker.result))
			{
				CAGE_ASSERT(matches(input, maker.result) != MatchResultEnum::Unmatched);
			}
			return maker.result;
		}

		class KeybindImpl : public Keybind
		{
		public:
			KeybindImpl(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults, Delegate<bool(const GenericInput &)> event) : config(config), defaults(defaults.begin(), defaults.end())
			{
				CAGE_ASSERT(!config.id.empty());
				CAGE_ASSERT(!findKeybind(config.id)); // must be unique
				CAGE_ASSERT(none(config.requiredFlags & config.forbiddenFlags));
				CAGE_ASSERT(any(config.devices));
				reset(); // make matchers from the defaults
				this->event = event;
				global.push_back(this);
			}

			~KeybindImpl()
			{
				auto it = std::find(global.begin(), global.end(), this);
				if (it != global.end())
					global.erase(it);
			}

			bool process(const GenericInput &input) const
			{
				if (input.has<input::WindowFocusLose>())
				{
					active = false;
					return false;
				}
				if (input.has<input::Tick>())
				{
					if (active)
						return event(input);
					return false;
				}
				for (const auto &it : matchers)
				{
					MatchResultEnum r = matches(input, it);
					switch (r)
					{
						case MatchResultEnum::Unmatched:
							continue;
						case MatchResultEnum::Activate:
							active = true;
							break;
						case MatchResultEnum::Deactivate:
							active = false;
							break;
					}
					return event(input);
				}
				return false;
			}

			const KeybindCreateConfig config;
			EventListener<bool(const GenericInput &)> listener;
			const std::vector<GenericInput> defaults;
			std::vector<Matcher> matchers;
			mutable bool active = false; // allows tick events
		};
	}

	String Keybind::name() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		// todo
		return {};
	}

	String Keybind::value(uint32 index) const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		CAGE_ASSERT(index < impl->matchers.size());
		// todo
		return {};
	}

	bool Keybind::process(const GenericInput &input) const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return impl->process(input);
	}

	uint32 Keybind::count() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return impl->matchers.size();
	}

	void Keybind::override(uint32 index, const GenericInput &input)
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		CAGE_ASSERT(index < impl->matchers.size());
		impl->matchers[index] = makeMatcher(impl->config, input);
	}

	void Keybind::add(const GenericInput &input)
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		const auto mt = makeMatcher(impl->config, input);
		if (!std::holds_alternative<std::monostate>(mt))
			impl->matchers.push_back(mt);
	}

	void Keybind::remove(uint32 index)
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		CAGE_ASSERT(index < impl->matchers.size());
		impl->matchers.erase(impl->matchers.begin() + index);
	}

	void Keybind::clear()
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		impl->matchers.clear();
	}

	void Keybind::reset()
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		clear();
		for (const auto &it : impl->defaults)
			add(it);
		CAGE_ASSERT(impl->matchers.size() == impl->defaults.size());
	}

	Holder<Keybind> newKeybind(const KeybindCreateConfig &config, const GenericInput &defaults, Delegate<bool(const GenericInput &)> event)
	{
		return newKeybind(config, PointerRange(&defaults, &defaults + 1), event);
	}

	Holder<Keybind> newKeybind(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults, Delegate<bool(const GenericInput &)> event)
	{
		return systemMemory().createImpl<Keybind, KeybindImpl>(config, defaults, event);
	}

	Keybind *findKeybind(const String &id)
	{
		for (KeybindImpl *it : global)
		{
			if (it->config.id == id)
				return it;
		}
		return nullptr;
	}

	void keybindsRegisterListeners(EventDispatcher<bool(const GenericInput &)> &dispatcher)
	{
		for (KeybindImpl *it : global)
		{
			it->listener.clear();
			it->listener.bind([it](const GenericInput &in) { return it->process(in); });
			it->listener.attach(dispatcher, it->config.eventOrder);
		}
	}

	void keybindsDispatchTick()
	{
		GenericInput g = input::Tick();
		for (KeybindImpl *it : global)
			it->process(g);
	}

	Holder<Ini> keybindsExport()
	{
		// todo
		return {};
	}

	void keybindsImport(const Ini *ini)
	{
		// todo
	}
}
