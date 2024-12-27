#include <map>
#include <vector>

#include <openxr/openxr.h>

#include <cage-core/ini.h>
#include <cage-core/serialization.h>

cage::PointerRange<const cage::uint8> controller_binding_0();
cage::PointerRange<const cage::uint8> controller_binding_1();
cage::PointerRange<const cage::uint8> controller_binding_2();
cage::PointerRange<const cage::uint8> controller_binding_3();
cage::PointerRange<const cage::uint8> controller_binding_4();
cage::PointerRange<const cage::uint8> controller_binding_5();
cage::PointerRange<const cage::uint8> controller_binding_6();

namespace cage
{
	namespace
	{
		std::map<String, Holder<Ini>> loadBindingsImpl()
		{
			std::map<String, Holder<Ini>> result;

			const auto &load = [&](PointerRange<const uint8> data)
			{
				Holder<Ini> ini = newIni();
				ini->importBuffer(bufferCast<const char>(data));
				result[ini->getString("profile", "profile")] = std::move(ini);
			};

			load(controller_binding_0());
			load(controller_binding_1());
			load(controller_binding_2());
			load(controller_binding_3());
			load(controller_binding_4());
			load(controller_binding_5());
			load(controller_binding_6());

			// todo load from file

			return result;
		}

		const std::map<String, Holder<Ini>> &loadBindings()
		{
			static const std::map<String, Holder<Ini>> bindings = loadBindingsImpl();
			return bindings;
		}

		void loadControllerBindingsImpl(XrInstance instance, const char *sideName, const String &profile, std::vector<XrActionSuggestedBinding> &suggestions, PointerRange<const XrAction> axesActions, PointerRange<const XrAction> butsActions, Ini *ini)
		{
			const auto &check = [&](XrResult result)
			{
				if (XR_SUCCEEDED(result))
					return;

				char s[XR_MAX_RESULT_STRING_SIZE] = {};
				xrResultToString(instance, result, s);
				if (s[0])
					CAGE_LOG_THROW(s);

				CAGE_THROW_ERROR(SystemError, "openxr error", result);
			};

			for (const String &section : ini->sections())
			{
				if (section == "profile")
					continue;
				if (!ini->getBool(section, sideName, true))
					continue;
				const String type = ini->getString(section, "type", "button");
				const String mapping = ini->getString(section, "mapping", "");
				const uint32 index = ini->getUint32(section, "index", m);
				XrPath path = 0;
				check(xrStringToPath(instance, (Stringizer() + "/user/hand/" + sideName + "/input/" + mapping).value.c_str(), &path));
				if (type == "button")
				{
					if (index >= butsActions.size())
						CAGE_THROW_ERROR(Exception, "index out of range");
					suggestions.push_back(XrActionSuggestedBinding{ butsActions[index], path });
				}
				else if (type == "axis")
				{
					if (index >= axesActions.size())
						CAGE_THROW_ERROR(Exception, "index out of range");
					suggestions.push_back(XrActionSuggestedBinding{ axesActions[index], path });
				}
				else
				{
					CAGE_LOG_THROW(Stringizer() + "type: " + type);
					CAGE_THROW_ERROR(Exception, "unsupported binding type");
				}
			}
		}
	}

	namespace privat
	{
		void loadControllerBindings(XrInstance instance, const char *sideName, std::map<String, std::vector<XrActionSuggestedBinding>> &suggestions, PointerRange<const XrAction> axesActions, PointerRange<const XrAction> butsActions)
		{
			const std::map<String, Holder<Ini>> &bindings = loadBindings();

			for (auto &it : bindings)
			{
				try
				{
					loadControllerBindingsImpl(instance, sideName, it.first, suggestions[it.first], axesActions, butsActions, +it.second);
				}
				catch (...)
				{
					CAGE_LOG_THROW(Stringizer() + "in profile: " + it.first);
					throw;
				}
			}
		}

		void controllerBindingsCheckUnused()
		{
			const std::map<String, Holder<Ini>> &bindings = loadBindings();
			for (const auto &it : bindings)
			{
				String section, item, value;
				if (it.second->anyUnused(section, item, value))
				{
					CAGE_LOG_THROW(Stringizer() + "section: " + section + ", item: " + item + ", value: " + value);
					CAGE_LOG_THROW(Stringizer() + "in profile: " + it.first);
					CAGE_THROW_ERROR(Exception, "unused item in openxr controller bindings configuration");
				}
			}
		}
	}
}
