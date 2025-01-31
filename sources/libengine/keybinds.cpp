#include <algorithm>
#include <tuple>
#include <variant>
#include <vector>

#include <GLFW/glfw3.h>

#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/string.h>
#include <cage-core/texts.h>
#include <cage-engine/guiBuilder.h>
#include <cage-engine/keybinds.h>

namespace cage
{
	namespace
	{
		std::vector<class KeybindImpl *> &global()
		{
			static std::vector<class KeybindImpl *> g;
			return g;
		}

		CAGE_FORCE_INLINE String finishName(String s)
		{
			return replace(trim(s), " ", " + ");
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

			CAGE_FORCE_INLINE KeybindModesFlags matchImpl(input::privat::BaseKey k, KeybindModesFlags activation) const
			{
				if ((k.key == key || k.key == m) && (checkFlags(k.mods) || activation == KeybindModesFlags::KeyRelease))
					return activation;
				return KeybindModesFlags::None;
			}

			template<class T>
			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const
			{
				if (input.has<T>())
					return matchImpl(input.get<T>(), inputKeybindMode(input));
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const { return match<input::KeyPress>(input) | match<input::KeyRepeat>(input) | match<input::KeyRelease>(input); }

			CAGE_FORCE_INLINE String value() const { return finishName(Stringizer() + getModifiersNames(requiredFlags) + " " + getKeyName(key)); }
		};

		struct ModifiersMatcher : public MatcherBase
		{
			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const
			{
				// todo this is wrong
				// i need to explicitly check for key press and release of the particular keys
				return checkFlags(inputModifiersFlags(input)) ? KeybindModesFlags::KeyPress : KeybindModesFlags::KeyRelease;
			}

			CAGE_FORCE_INLINE String value() const { return finishName(getModifiersNames(requiredFlags)); }
		};

		struct MouseMatcher : public MatcherBase
		{
			MouseButtonsFlags button = MouseButtonsFlags::None;

			CAGE_FORCE_INLINE KeybindModesFlags matchImpl(input::privat::BaseMouse k, KeybindModesFlags activation) const
			{
				if ((any(k.buttons & button) || k.buttons == (MouseButtonsFlags)m) && (checkFlags(k.mods) || activation == KeybindModesFlags::MouseRelease))
					return activation;
				return KeybindModesFlags::None;
			}

			template<class T>
			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const
			{
				if (input.has<T>())
					return matchImpl(input.get<T>(), inputKeybindMode(input));
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const { return match<input::MousePress>(input) | match<input::MouseDoublePress>(input) | match<input::MouseRelease>(input); }

			CAGE_FORCE_INLINE String value() const { return finishName(Stringizer() + getModifiersNames(requiredFlags) + " " + getButtonsNames(button)); }
		};

		struct WheelMatcher : public MatcherBase
		{
			sint32 direction = 0;

			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const
			{
				if (input.has<input::MouseWheel>())
				{
					const input::MouseWheel w = input.get<input::MouseWheel>();
					if (!checkFlags(w.mods))
						return KeybindModesFlags::None;
					if (direction == 0)
						return KeybindModesFlags::WheelScroll;
					if (direction > 0 && w.wheel > 0)
						return KeybindModesFlags::WheelRoll;
					if (direction < 0 && w.wheel < 0)
						return KeybindModesFlags::WheelRoll;
				}
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE String value() const
			{
				String w;
				if (direction > 0)
					w = "WhUp";
				else if (direction < 0)
					w = "WhDn";
				else
					w = "Wheel";
				return finishName(Stringizer() + getModifiersNames(requiredFlags) + " " + w);
			}
		};

		using Matcher = std::variant<std::monostate, KeyboardMatcher, ModifiersMatcher, MouseMatcher, WheelMatcher>;

		CAGE_FORCE_INLINE bool operator==(const Matcher &a, const Matcher &b)
		{
			if (a.index() != b.index())
				return false;
			return std::visit(
				[](const auto &a, const auto &b) -> bool
				{
					using T = std::decay_t<decltype(a)>;
					if constexpr (std::is_same_v<T, std::monostate>)
						return true;
					else
					{
						// using operator == leads to stack overflow
						static_assert(std::is_trivially_copyable_v<T>);
						return detail::memcmp(&a, &b, sizeof(T)) == 0;
					}
				},
				a, b);
		}

		CAGE_FORCE_INLINE KeybindModesFlags matches(const GenericInput &input, const Matcher &matcher)
		{
			return std::visit(
				[input](const auto &mt) -> KeybindModesFlags
				{
					if constexpr (std::is_same_v<std::decay_t<decltype(mt)>, std::monostate>)
						return KeybindModesFlags::None;
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
				if (config.exactFlags)
					r.forbiddenFlags |= ~r.requiredFlags;
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
				if (any(k.mods & config.forbiddenFlags))
					return;
				if (any(config.devices & KeybindDevicesFlags::WheelScroll))
					result = WheelMatcher{ base(k.mods), 0 };
				if (any(config.devices & KeybindDevicesFlags::WheelRoll) && k.wheel > 0)
					result = WheelMatcher{ base(k.mods), 1 };
				if (any(config.devices & KeybindDevicesFlags::WheelRoll) && k.wheel < 0)
					result = WheelMatcher{ base(k.mods), -1 };
			}

			template<class T>
			CAGE_FORCE_INLINE void make(const GenericInput &input)
			{
				if (!std::holds_alternative<std::monostate>(result))
					return;
				if (input.has<T>())
					makeImpl(input.get<T>());
			}

			CAGE_FORCE_INLINE void make(const GenericInput &input)
			{
				if (config.devices == KeybindDevicesFlags::Modifiers)
					return makeModifiersMatcher(input);
				make<input::KeyPress>(input);
				make<input::KeyRepeat>(input);
				make<input::KeyRelease>(input);
				make<input::MousePress>(input);
				make<input::MouseDoublePress>(input);
				make<input::MouseRelease>(input);
				make<input::MouseWheel>(input);
			}

			CAGE_FORCE_INLINE void makeModifiersMatcher(const GenericInput &input)
			{
				if (input.has<input::KeyPress>())
				{
					const input::KeyPress in = input.get<input::KeyPress>();
					if (none(in.mods & config.forbiddenFlags))
						result = ModifiersMatcher{ base(in.mods) };
				}
			}
		};

		Matcher makeMatcher(const KeybindCreateConfig &config, const GenericInput &input)
		{
			Maker maker{ config };
			maker.make(input);
			if (!std::holds_alternative<std::monostate>(maker.result))
			{
				CAGE_ASSERT(any(matches(input, maker.result)));
			}
			return maker.result;
		}

		const KeybindCreateConfig &validateConfig(const KeybindCreateConfig &config)
		{
			CAGE_ASSERT(!config.id.empty());
			CAGE_ASSERT(!findKeybind(config.id)); // must be unique
			CAGE_ASSERT(none(config.requiredFlags & config.forbiddenFlags));
			CAGE_ASSERT(any(config.devices));
			CAGE_ASSERT(none(config.devices & KeybindDevicesFlags::WheelRoll) || none(config.devices & KeybindDevicesFlags::WheelScroll)); // these two flags are mutually exclusive
			CAGE_ASSERT(none(config.devices & KeybindDevicesFlags::Modifiers) || config.devices == KeybindDevicesFlags::Modifiers); // modifiers is exclusive with all other flags
			CAGE_ASSERT(any(config.modes));
			return config;
		}

		std::vector<Matcher> makeDefaults(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults)
		{
			std::vector<Matcher> res;
			for (const auto &it : defaults)
			{
				const auto mt = makeMatcher(config, it);
				if (!std::holds_alternative<std::monostate>(mt))
					res.push_back(mt);
			}
			CAGE_ASSERT(res.size() == defaults.size());
			return res;
		}

		bool guiUpdateGlobal(uintPtr ptr, const GenericInput &);

		EventListener<bool(const GenericInput &)> assignmentListener;

		class KeybindImpl : public Keybind
		{
		public:
			KeybindImpl(const KeybindCreateConfig &config_, PointerRange<const GenericInput> defaults_, Delegate<bool(const GenericInput &)> event_) : config(validateConfig(config_)), defaults(makeDefaults(config_, defaults_)), textId(HashString(config.id))
			{
				reset(); // make matchers from the defaults
				this->event = event_;
				global().push_back(this);
			}

			~KeybindImpl()
			{
				auto it = std::find(global().begin(), global().end(), this);
				if (it != global().end())
					global().erase(it);
			}

			bool process(const GenericInput &input) const
			{
				if (input.has<input::WindowFocusLose>())
				{
					active = false;
					return false;
				}
				if (input.has<input::GameTick>())
				{
					if (active && any(config.modes & KeybindModesFlags::GameTick) && event)
						event(input);
					return false; // ticks always propagate
				}
				if (input.has<input::EngineTick>())
				{
					if (active && any(config.modes & KeybindModesFlags::EngineTick) && event)
						event(input);
					if (autoDeactivate)
					{
						active = false;
						autoDeactivate = false;
					}
					return false; // ticks always propagate
				}
				for (const auto &it : matchers)
				{
					const KeybindModesFlags r = matches(input, it);
					if (any(r & (KeybindModesFlags::KeyPress | KeybindModesFlags::MousePress)))
						active = true;
					if (any(r & (KeybindModesFlags::WheelRoll | KeybindModesFlags::WheelScroll)))
					{
						active = true;
						autoDeactivate = true;
					}
					if (any(r & (KeybindModesFlags::KeyRelease | KeybindModesFlags::MouseRelease)))
						active = false;
					if (any(r & config.modes))
					{
						if (event)
						{
							const bool p = event(input);
							if (none(r & (KeybindModesFlags::KeyRelease | KeybindModesFlags::MouseRelease))) // release always propagates
								return p;
						}
						return false;
					}
				}
				return false;
			}

			const KeybindCreateConfig config;
			EventListener<bool(const GenericInput &)> listener;
			const std::vector<Matcher> defaults;
			std::vector<Matcher> matchers;
			const uint32 textId = 0;
			mutable bool active = false; // allows tick events
			mutable bool autoDeactivate = false; // automatically deactivates after first engine tick
			Entity *guiEnt = nullptr;
			uint32 assigningIndex = m;

			void makeGui()
			{
				CAGE_ASSERT(guiEnt);
				detail::guiDestroyChildrenRecursively(guiEnt);
				Holder<GuiBuilder> g = newGuiBuilder(guiEnt);
				auto _ = g->leftRowStretchRight();
				for (uint32 i = 0; i < count(); i++)
				{
					auto _ = g->button().event(Delegate<bool(const GenericInput &)>().bind<uintPtr, &guiUpdateGlobal>(uintPtr(this) + i)).text(value(i));
					if (assigningIndex == i)
						_.accent(Vec4(50, 230, 255, 100) / 255);
				}
				{
					auto _ = g->row(0.5);
					g->button().disabled(count() >= 3).event(Delegate<bool(const GenericInput &)>().bind<KeybindImpl, &KeybindImpl::guiAdd>(this)).image(HashString("cage/texture/keybindAdd.png")).tooltip<HashString("cage/keybinds/add"), "Add">().size(Vec2(28));
					g->button().disabled(count() == 0).event(Delegate<bool(const GenericInput &)>().bind<KeybindImpl, &KeybindImpl::guiRemove>(this)).image(HashString("cage/texture/keybindRemove.png")).tooltip<HashString("cage/keybinds/remove"), "Remove">().size(Vec2(28));
				}
				{
					auto _ = g->rightRow(0.5);
					g->button().disabled(count() == 0).event(Delegate<bool(const GenericInput &)>().bind<KeybindImpl, &KeybindImpl::guiClear>(this)).image(HashString("cage/texture/keybindClear.png")).tooltip<HashString("cage/keybinds/clear"), "Clear">().size(Vec2(28));
					g->button().disabled(defaults == matchers).event(Delegate<bool(const GenericInput &)>().bind<KeybindImpl, &KeybindImpl::guiReset>(this)).image(HashString("cage/texture/keybindReset.png")).tooltip<HashString("cage/keybinds/reset"), "Reset">().size(Vec2(28));
				}
			}

			bool guiUpdate(uint32 index, const GenericInput &)
			{
				assigningIndex = index;
				makeGui();
				assignmentListener.bind(Delegate<bool(const GenericInput &in)>().bind<KeybindImpl, &KeybindImpl::guiAssign>(this));
				return true;
			}

			void cancel()
			{
				assignmentListener.unbind();
				assigningIndex = m;
				makeGui();
			}

			bool guiAssign(const GenericInput &in)
			{
				CAGE_ASSERT(assigningIndex < matchers.size());
				if (in.has<input::WindowFocusLose>())
				{
					cancel();
					return false;
				}
				if (in.has<input::KeyRelease>() || in.has<input::KeyRepeat>())
					return false;
				if (in.has<input::KeyPress>() && config.devices != KeybindDevicesFlags::Modifiers)
				{
					const input::KeyPress k = in.get<input::KeyPress>();
					switch (k.key)
					{
						case GLFW_KEY_LEFT_SHIFT:
						case GLFW_KEY_LEFT_CONTROL:
						case GLFW_KEY_LEFT_ALT:
						case GLFW_KEY_LEFT_SUPER:
						case GLFW_KEY_RIGHT_SHIFT:
						case GLFW_KEY_RIGHT_CONTROL:
						case GLFW_KEY_RIGHT_ALT:
						case GLFW_KEY_RIGHT_SUPER:
							return false;
					}
				}
				if (in.has<input::MouseDoublePress>() || in.has<input::MouseRelease>() || in.has<input::MouseMove>() || in.has<input::MouseRelativeMove>())
					return false;
				if (in.has<input::EngineTick>() || in.has<input::GameTick>())
					return false;
				override(assigningIndex, in);
				cancel();
				return true;
			}

			bool guiAdd(const GenericInput &)
			{
				add();
				makeGui();
				return true;
			}

			bool guiRemove(const GenericInput &)
			{
				remove(count() - 1);
				makeGui();
				return true;
			}

			bool guiClear(const GenericInput &)
			{
				clear();
				makeGui();
				return true;
			}

			bool guiReset(const GenericInput &)
			{
				reset();
				makeGui();
				return true;
			}

			CAGE_FORCE_INLINE auto cmp() const { return std::tuple<sint32, String, String>(config.displayOrder, textsGet(textId), config.id); }
		};

		bool guiUpdateGlobal(uintPtr ptr, const GenericInput &in)
		{
			const uint32 index = ptr % alignof(KeybindImpl);
			ptr -= index;
			KeybindImpl *impl = (KeybindImpl *)ptr;
			return impl->guiUpdate(index, in);
		}
	}

	const String &Keybind::id() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return impl->config.id;
	}

	String Keybind::name() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return textsGet(impl->textId, impl->config.id);
	}

	String Keybind::value() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		if (!impl->matchers.empty())
			return value(0);
		return "???";
	}

	String Keybind::value(uint32 index) const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		CAGE_ASSERT(index < impl->matchers.size());
		return std::visit(
			[](const auto &mt) -> String
			{
				if constexpr (std::is_same_v<std::decay_t<decltype(mt)>, std::monostate>)
					return "";
				else
					return mt.value();
			},
			impl->matchers[index]);
	}

	bool Keybind::process(const GenericInput &input) const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return impl->process(input);
	}

	void Keybind::setActive(bool active, bool autoDeactivate)
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		impl->active = active;
		impl->autoDeactivate = autoDeactivate;
	}

	bool Keybind::active() const
	{
		const KeybindImpl *impl = (const KeybindImpl *)this;
		return impl->active;
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
		if (input)
		{
			const auto mt = makeMatcher(impl->config, input);
			if (!std::holds_alternative<std::monostate>(mt))
				impl->matchers.push_back(mt);
		}
		else
			impl->matchers.push_back({});
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
		impl->matchers = impl->defaults;
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
		for (KeybindImpl *it : global())
		{
			if (it->config.id == id)
				return it;
		}
		return nullptr;
	}

	void keybindsRegisterListeners(EventDispatcher<bool(const GenericInput &)> &dispatcher)
	{
		assignmentListener.attach(dispatcher, -328655984);
		for (KeybindImpl *it : global())
		{
			it->listener.unbind();
			it->listener.bind([it](const GenericInput &in) { return it->process(in); });
			it->listener.attach(dispatcher, it->config.eventOrder);
		}
	}

	void keybindsGuiWidget(GuiBuilder *g, Keybind *k_)
	{
		KeybindImpl *k = (KeybindImpl *)k_;
		auto _ = g->empty();
		k->guiEnt = _.entity();
		k->makeGui();
	}

	void keybindsGuiTable(GuiBuilder *g, const String &filterPrefix)
	{
		std::vector<KeybindImpl *> tmp;
		tmp.reserve(global().size());
		for (KeybindImpl *k : global())
		{
			if (isPattern(k->config.id, filterPrefix, "", ""))
				tmp.push_back(k);
		}
		std::sort(tmp.begin(), tmp.end(), [](const KeybindImpl *a, const KeybindImpl *b) -> bool { return a->cmp() < b->cmp(); });

		auto _ = g->verticalTable(2);
		for (KeybindImpl *k : tmp)
		{
			g->label().update([k](Entity *e) { e->value<GuiTextFormatComponent>().color = k->active ? Vec3(0.5, 0.9, 1) : Vec3::Nan(); }).text(k->textId, k->config.id);
			keybindsGuiWidget(g, k);
		}
	}

	namespace
	{
		String toString(const Matcher &mt)
		{
			return std::visit(
				[](const auto &a) -> String
				{
					using T = std::decay_t<decltype(a)>;
					//if constexpr (std::is_same_v<T, std::monostate>)
					//	return "";
					if constexpr (std::is_same_v<T, KeyboardMatcher>)
						return Stringizer() + "key " + a.key + " " + (uint32)a.requiredFlags + " " + (uint32)~a.forbiddenFlags;
					if constexpr (std::is_same_v<T, ModifiersMatcher>)
						return Stringizer() + "mods " + (uint32)a.requiredFlags + " " + (uint32)~a.forbiddenFlags;
					if constexpr (std::is_same_v<T, MouseMatcher>)
						return Stringizer() + "mouse " + (uint32)a.button + " " + (uint32)a.requiredFlags + " " + (uint32)~a.forbiddenFlags;
					if constexpr (std::is_same_v<T, WheelMatcher>)
						return Stringizer() + "wheel " + a.direction + " " + (uint32)a.requiredFlags + " " + (uint32)~a.forbiddenFlags;
				},
				mt);
		}

		MatcherBase baseFromString(String &s)
		{
			const uint32 r = toUint32(split(s));
			const uint32 f = toUint32(split(s));
			return MatcherBase{ (ModifiersFlags)r, ~(ModifiersFlags)f };
		}

		Matcher fromString(const String &str)
		{
			if (str.empty())
				return {};
			String s = str;
			const String type = split(s);
			if (type == "key")
			{
				const uint32 k = toUint32(split(s));
				return KeyboardMatcher{ baseFromString(s), k };
			}
			else if (type == "mods")
			{
				return ModifiersMatcher{ baseFromString(s) };
			}
			else if (type == "mouse")
			{
				const uint32 b = toUint32(split(s));
				return MouseMatcher{ baseFromString(s), (MouseButtonsFlags)b };
			}
			else if (type == "wheel")
			{
				const sint32 d = toUint32(split(s));
				return WheelMatcher{ baseFromString(s), d };
			}
			else
				return {};
		}
	}

	Holder<Ini> keybindsExport()
	{
		Holder<Ini> ini = newIni();
		for (KeybindImpl *k : global())
		{
			ini->set(k->config.id, "count", Stringizer() + k->matchers.size());
			uint32 i = 0;
			for (const Matcher &mt : k->matchers)
			{
				ini->set(k->config.id, Stringizer() + i, toString(mt));
				i++;
			}
		}
		return ini;
	}

	void keybindsImport(const Ini *ini, bool updateGui)
	{
		for (KeybindImpl *k : global())
		{
			try
			{
				if (ini->sectionExists(k->config.id))
				{
					k->clear();
					for (const String &it : ini->items(k->config.id))
					{
						if (it != "count")
							k->matchers.push_back(fromString(ini->get(k->config.id, it)));
					}
				}
				else
					k->reset();
				if (updateGui)
					k->makeGui();
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Warning, "keybinds", Stringizer() + "error importing keybinds for: " + k->config.id);
				k->reset();
			}
		}
	}

	void keybindsResetAll(bool updateGui)
	{
		for (KeybindImpl *k : global())
		{
			k->reset();
			if (updateGui)
				k->makeGui();
		}
	}

	KeybindModesFlags inputKeybindMode(const GenericInput &in)
	{
		if (in.has<input::KeyPress>())
			return KeybindModesFlags::KeyPress;
		if (in.has<input::KeyRepeat>())
			return KeybindModesFlags::KeyRepeat;
		if (in.has<input::KeyRelease>())
			return KeybindModesFlags::KeyRelease;
		if (in.has<input::MousePress>())
			return KeybindModesFlags::MousePress;
		if (in.has<input::MouseDoublePress>())
			return KeybindModesFlags::MouseDouble;
		if (in.has<input::MouseRelease>())
			return KeybindModesFlags::MouseRelease;
		if (in.has<input::MouseWheel>())
			return KeybindModesFlags::WheelScroll;
		if (in.has<input::GameTick>())
			return KeybindModesFlags::GameTick;
		if (in.has<input::EngineTick>())
			return KeybindModesFlags::EngineTick;
		return KeybindModesFlags::None;
	}

	namespace
	{
		template<class T>
		CAGE_FORCE_INLINE ModifiersFlags inputModifiersImpl(const GenericInput &input)
		{
			if (input.has<T>())
				return input.get<T>().mods;
			return ModifiersFlags::None;
		}
	}

	ModifiersFlags inputModifiersFlags(const GenericInput &in)
	{
		return inputModifiersImpl<input::KeyPress>(in) | inputModifiersImpl<input::KeyRepeat>(in) | inputModifiersImpl<input::KeyRelease>(in) | inputModifiersImpl<input::MousePress>(in) | inputModifiersImpl<input::MouseDoublePress>(in) | inputModifiersImpl<input::MouseRelease>(in) | inputModifiersImpl<input::MouseWheel>(in) | inputModifiersImpl<input::MouseMove>(in) | inputModifiersImpl<input::MouseRelativeMove>(in);
	}
}
