#ifdef CAGE_USE_STEAM_SOCKETS

	#include <absl/log/globals.h>
	#include <absl/log/initialize.h>
	#include <absl/log/log.h>
	#include <absl/log/log_sink.h>
	#include <absl/log/log_sink_registry.h>

	#include <cage-core/core.h>

namespace cage
{
	namespace
	{
		class AbslLogSink final : public absl::LogSink
		{
		public:
			AbslLogSink()
			{
				absl::EnableLogPrefix(false);
				absl::SetStderrThreshold(absl::LogSeverity::kFatal);
				absl::AddLogSink(this);
				//absl::InitializeLog();
				//LOG(INFO) << "absl logging redirected to cage";
			}

			~AbslLogSink() { absl::RemoveLogSink(this); }

			void Send(const absl::LogEntry &entry) override
			{
				std::source_location location = std::source_location::current(); // todo copy values from the entry
				const cage::SeverityEnum severity = [](absl::LogSeverity severity)
				{
					switch (severity)
					{
						case absl::LogSeverity::kInfo:
							return SeverityEnum::Info;
						case absl::LogSeverity::kWarning:
							return SeverityEnum::Warning;
						case absl::LogSeverity::kError:
							return SeverityEnum::Error;
						case absl::LogSeverity::kFatal:
						default:
							return SeverityEnum::Critical;
					}
				}(entry.log_severity());
				cage::privat::makeLog(location, severity, "absl", entry.text_message(), false, false);
			}
		};
	}

	namespace privat
	{
		CAGE_API_EXPORT void initializeAbslLogSink()
		{
			static AbslLogSink logSink;
		}
	}
}

#endif // CAGE_USE_STEAM_SOCKETS
