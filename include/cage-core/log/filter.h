namespace cage
{
	inline bool logFilterPolicyPass(const detail::loggerInfo &info)
	{
		return true;
	}

	inline bool logFilterPolicyThread(const detail::loggerInfo &info)
	{
		return info.createThreadId == info.currentThreadId;
	}

	template<severityEnum Severity>
	inline bool logFilterPolicySeverity(const detail::loggerInfo &info)
	{
		return info.severity >= Severity;
	}

	template<severityEnum Severity, const char *Components, bool SameThread, bool Continuous>
	inline bool logFilterPolicyCustom(const detail::loggerInfo &info)
	{
		if (info.severity < Severity)
			return false;
		if (SameThread && (info.createThreadId != info.currentThreadId))
			return false;
		if (!Continuous && info.continuous)
			return false;
		if (Components)
		{
			string comps = Components;
			bool ok = false;
			string a = comps.split();
			while (!a.empty())
			{
				if (a == info.component)
				{
					ok = true;
					break;
				}
				a = comps.split();
			}
			if (!ok)
				return false;
		}
		return true;
	}
}
