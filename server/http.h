#pragma once
#include <map>
#include <regex>

class HTTP {
public:
	enum Type { REQUEST, RESPONSE } type;
	enum Method {
		CONNECT,
		DELETE,
		GET,
		HEAD,
		OPTIONS,
		PATCH,
		POST,
		PUT,
		TRACE
	};
	struct RequestType {
		Method method = GET;
		std::string path = "/";
		std::string version = "HTTP/1.1";
	};
	struct ResponseType {
		std::string version = "HTTP/1.1";
		int status = 200;
		std::string reason = "OK";
	};
	RequestType request;
	ResponseType response;
	std::map<std::string, std::string> headers;
	std::string body;

	void parse(const std::string& str) {
		int second_line = str.find("\n") + 1;
		std::string first_line = str.substr(0, second_line - 2);
		int headers_end = str.find("\r\n\r\n") + 4;
		std::string headers = str.substr(second_line, headers_end - 2 - second_line);
		body = str.substr(headers_end, str.size() - headers_end);
		
		parse_first_line(first_line);
		parse_headers(headers);
	}

	std::string to_string() {
		std::string str;
		if (type == RESPONSE) {
			str += response.version + " " + std::to_string(response.status) + " " + response.reason + "\r\n";
			for (auto& header : headers) {
				str += header.first + ": " + header.second + "\r\n";
			}
			str += "\r\n";
			str += body;
			return str;
		}
		else {
			str += method_to_string(request.method) + " " + request.path + " " + request.version + "\r\n";
			for (auto& header : headers) {
				str += header.first + ": " + header.second + "\r\n";
			}
			str += "\r\n";
			str += body;
			return str;
		}
	}

	Method parse_method(const std::string& method) {
		if (method == "CONNECT") return CONNECT;
		if (method == "DELETE") return DELETE;
		if (method == "GET") return GET;
		if (method == "HEAD") return HEAD;
		if (method == "OPTIONS") return OPTIONS;
		if (method == "PATCH") return PATCH;
		if (method == "POST") return POST;
		if (method == "PUT") return PUT;
		if (method == "TRACE") return TRACE;
		return GET;
	}

	std::string method_to_string(Method method) {
		switch (method) {
		case CONNECT: return "CONNECT";
		case DELETE: return "DELETE";
		case GET: return "GET";
		case HEAD: return "HEAD";
		case OPTIONS: return "OPTIONS";
		case PATCH: return "PATCH";
		case POST: return "POST";
		case PUT: return "PUT";
		case TRACE: return "TRACE";
		default: return "GET";
		}
	}

private:
	void parse_first_line(const std::string& line) {
		std::cout << line << std::endl;
		if (line.substr(0, 4) == "HTTP") {
			// Response
			std::regex r("HTTP/([0-9.]+) ([0-9]+) (.*)");

			std::smatch m;
			if (std::regex_match(line, m, r)) {
				response.version = m[1];
				response.status = std::stoi(m[2]);
				response.reason = m[3];
			}
			type = RESPONSE;
		}
		else {
			// Request
			std::regex r("([A-Z]+) (.*) HTTP/([0-9.]+)");

			std::smatch m;
			if (std::regex_match(line, m, r)) {
				request.method = parse_method(m[1]);
				request.path = m[2];
				request.version = m[3];
			}
			type = REQUEST;
		}
	}

	void parse_headers(const std::string& lines) {
		std::regex r("(.*?): (.*?)\\r\\n");

		std::sregex_iterator it(lines.begin(), lines.end(), r);
		std::sregex_iterator end;
		while (it != end) {
			headers[it->str(1)] = it->str(2);
			++it;
		}
	}
};
