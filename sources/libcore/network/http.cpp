#include <cpr/cpr.h>

#include <cage-core/networkHttp.h>

namespace cage
{
	namespace
	{
		cpr::Header header(std::unordered_map<std::string, std::string> &&a)
		{
			return { a.begin(), a.end() };
		}
		cpr::Parameters parameters(std::unordered_map<std::string, std::string> &&a)
		{
			cpr::Parameters r;
			for (auto &it : a)
				r.Add({ it.first, std::move(it.second) });
			return r;
		}
		cpr::Multipart multipart(std::unordered_map<std::string, std::string> &&ms, std::unordered_map<std::string, std::pair<std::string, std::string>> &&fs)
		{
			cpr::Multipart r{};
			r.parts.reserve(ms.size() + fs.size());
			for (auto &it : ms)
				r.parts.push_back({ it.first, std::move(it.second) });
			for (auto &it : fs)
				r.parts.push_back({ it.first, cpr::Buffer(it.second.second.begin(), it.second.second.end(), std::move(it.second.first)) });
			return r;
		}
	}

	HttpResponse http(HttpRequest &&request)
	{
		try
		{
			cpr::Response r;
			cpr::Session s;
			s.SetUrl(cpr::Url(request.url));
			if (!request.headers.empty())
				s.UpdateHeader(header(std::move(request.headers)));
			if (!request.parameters.empty())
				s.SetParameters(parameters(std::move(request.parameters)));
			if (!request.multipart.empty() || !request.files.empty())
			{
				s.SetMultipart(multipart(std::move(request.multipart), std::move(request.files)));
				r = s.Post();
			}
			else
				r = s.Get();

			if (r.status_code != 200)
			{
				HttpError err(std::source_location::current(), SeverityEnum::Error, "http error");
				if (r.reason.empty())
					err.response = std::move(r.error.message);
				else
					err.response = std::move(r.reason);
				err.url = request.url;
				err.statusCode = r.status_code;
				err.makeLog();
				throw err;
			}

			HttpResponse o;
			o.body = std::move(r.text);
			o.statusCode = r.status_code;
			return o;
		}
		catch (const std::exception &e)
		{
			HttpError err(std::source_location::current(), SeverityEnum::Error, "http error (std::exception)");
			err.response = std::move(e.what());
			err.url = request.url;
			err.makeLog();
			throw err;
		}
	}

	void HttpError::log() const
	{
		::cage::privat::makeLog(location, SeverityEnum::Note, "exception", url.c_str(), false, false);
		::cage::privat::makeLog(location, SeverityEnum::Note, "exception", Stringizer() + "status code: " + statusCode, false, false);
		::cage::privat::makeLog(location, SeverityEnum::Note, "exception", response.c_str(), false, false);
		::cage::privat::makeLog(location, severity, "exception", +message, false, false);
	}
}
