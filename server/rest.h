#pragma once
#include <functional>

#include "http.h"

class Endpoint {
public:
	std::map<HTTP::Method, std::function<HTTP(const HTTP&)>> methods;
	std::map<std::string, std::shared_ptr<Endpoint>> endpoints;

	HTTP call(HTTP request) {
		auto it = std::min(request.request.path.find("/", 1), request.request.path.find("?"));
		std::string part = request.request.path.substr(1, it - 1);

		for (auto& endpoint : endpoints) {
			if (endpoint.first == part) {
				auto request_part = request;
				if (it == std::string::npos) {
					request_part.request.path = "/";
				}
				else {
					request_part.request.path = request.request.path.substr(it - 1);
				}
				return endpoint.second->call(request_part);
			}
		}

		for (auto& method : methods) {
			if (method.first == request.request.method) {
				return method.second(request);
			}
		}

		HTTP default_response;
		default_response.type = HTTP::Type::RESPONSE;
		default_response.response.version = "HTTP/1.1";
		default_response.response.status = 404;
		default_response.response.reason = "Not Found";
		default_response.headers["Connection"] = "close";
		default_response.body = "Not Found";
		default_response.headers["Content-Length"] = std::to_string(default_response.body.size());
		return default_response;
	}
};
