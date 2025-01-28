#include <algorithm>
#include <tuple>
#include <variant>
#include <vector>

#include <GLFW/glfw3.h>

#include <cage-core/hashString.h>
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
			return replace(trim(s), " ", "+");
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
					return matchImpl(input.get<T>(), keybindMode(input));
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const { return match<input::KeyPress>(input) | match<input::KeyRepeat>(input) | match<input::KeyRelease>(input); }

			CAGE_FORCE_INLINE String value() const { return finishName(Stringizer() + getModifiersNames(requiredFlags) + " " + getKeyName(key)); }
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
					return matchImpl(input.get<T>(), keybindMode(input));
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const { return match<input::MousePress>(input) | match<input::MouseDoublePress>(input) | match<input::MouseRelease>(input); }

			CAGE_FORCE_INLINE String value() const { return finishName(Stringizer() + getModifiersNames(requiredFlags) + " " + getButtonsNames(button)); }
		};

		struct WheelMatcher : public MatcherBase
		{
			CAGE_FORCE_INLINE KeybindModesFlags match(const GenericInput &input) const
			{
				if (input.has<input::MouseWheel>())
				{
					if (checkFlags(input.get<input::MouseWheel>().mods))
						return KeybindModesFlags::WheelScroll;
				}
				return KeybindModesFlags::None;
			}

			CAGE_FORCE_INLINE String value() const { return finishName(Stringizer() + getModifiersNames(requiredFlags) + " WHEEL"); }
		};

		using Matcher = std::variant<std::monostate, KeyboardMatcher, MouseMatcher, WheelMatcher>;

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

			CAGE_FORCE_INLINE void make(const GenericInput &input)
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
				CAGE_ASSERT(any(matches(input, maker.result)));
			}
			return maker.result;
		}

		bool guiUpdateGlobal(uintPtr ptr, const GenericInput &);

		EventListener<bool(const GenericInput &)> assignmentListener;

		class KeybindImpl : public Keybind
		{
		public:
			KeybindImpl(const KeybindCreateConfig &config, PointerRange<const GenericInput> defaults, Delegate<bool(const GenericInput &)> event) : config(config), defaults(defaults.begin(), defaults.end()), textId(HashString(config.id))
			{
				CAGE_ASSERT(!config.id.empty());
				CAGE_ASSERT(!findKeybind(config.id)); // must be unique
				CAGE_ASSERT(none(config.requiredFlags & config.forbiddenFlags));
				CAGE_ASSERT(any(config.devices));
				CAGE_ASSERT(any(config.modes));
				reset(); // make matchers from the defaults
				this->event = event;
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
				if (active)
				{
					if (input.has<input::GameTick>() && any(config.modes & KeybindModesFlags::GameTick))
						return event(input);
					if (input.has<input::EngineTick>() && any(config.modes & KeybindModesFlags::EngineTick))
						return event(input);
				}
				for (const auto &it : matchers)
				{
					const KeybindModesFlags r = matches(input, it);
					if (any(r & (KeybindModesFlags::KeyPress | KeybindModesFlags::MousePress)))
						active = true;
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
			const std::vector<GenericInput> defaults;
			std::vector<Matcher> matchers;
			const uint32 textId = 0;
			mutable bool active = false; // allows tick events

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
					g->button().event(Delegate<bool(const GenericInput &)>().bind<KeybindImpl, &KeybindImpl::guiReset>(this)).image(HashString("cage/texture/keybindReset.png")).tooltip<HashString("cage/keybinds/reset"), "Reset">().size(Vec2(28));
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
				if (in.has<input::WindowFocusLose>())
				{
					cancel();
					return false;
				}
				if (in.has<input::KeyRelease>() || in.has<input::KeyRepeat>())
					return false;
				if (in.has<input::KeyPress>())
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

	void Keybind::setActive(bool a)
	{
		KeybindImpl *impl = (KeybindImpl *)this;
		impl->active = a;
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
		clear();
		for (const auto &it : impl->defaults)
			add(it);
		CAGE_ASSERT(impl->matchers.size() == impl->defaults.size());
	}

	KeybindModesFlags keybindMode(const GenericInput &in)
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

	void keybindsDispatchGameTick()
	{
		GenericInput g = input::GameTick();
		for (KeybindImpl *it : global())
			it->process(g);
	}

	void keybindsDispatchEngineTick()
	{
		GenericInput g = input::EngineTick();
		for (KeybindImpl *it : global())
			it->process(g);
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
