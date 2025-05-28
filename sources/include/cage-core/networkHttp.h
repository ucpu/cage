#ifndef guard_networkHttp_h_f254rloiuigfrde
#define guard_networkHttp_h_f254rloiuigfrde

#include <string>
#include <unordered_map>
#include <vector>

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API HttpRequest
	{
		std::string url;
		std::unordered_map<std::string, std::string> headers, parameters, multipart;
		std::unordered_map<std::string, std::pair<std::string, std::string>> files; // form-name -> file-name, file-content
	};

	struct CAGE_CORE_API HttpResponse
	{
		std::string body;
		uint32 statusCode = 0;
	};

	CAGE_CORE_API HttpResponse http(HttpRequest &&request);

	struct CAGE_CORE_API HttpError : public Exception
	{
		using Exception::Exception;
		void log() const override;

		std::string url;
		std::string response;
		uint32 statusCode = 0;
	};
}

#endif // guard_networkHttp_h_f254rloiuigfrde
