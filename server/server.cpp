#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <cstring>
#include <memory>
#include <functional>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "tcp-server.h"
#include "http.h"
#include "rest.h"
#include "utility.h"
#include "notifications.h"

namespace API {
	bool check_auth(const HTTP& http) {
		auto it = http.headers.find("Authorization");
		if (it == http.headers.end()) {
			return false;
		}
		return it->second == "0d067646-9ca4-43ee-b32d-0d15d9cd6e67";
	}

	void connection_handler(int client_socket_fd, std::shared_ptr<Notifications> notifications) {
		try {
			char buffer[2048];
			int length = recv(client_socket_fd, buffer, 2048, 0);
			std::string request = std::string(buffer, length);

			HTTP http;
			http.parse(request);

			Endpoint rest;
			{ // GET /feed
				std::shared_ptr<Endpoint> endpoint = std::make_shared<Endpoint>();
				endpoint->methods[HTTP::Method::GET] = [notifications](const HTTP& http) {
					HTTP response;
					response.type = HTTP::Type::RESPONSE;
					response.headers["Connection"] = "close";
					{
						std::string rss_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
						rss_xml += "<rss version=\"2.0\">";
						rss_xml += "<channel>";
						rss_xml += "<title>Notifications</title>";
						rss_xml += "<link>http://100.76.115.82:10000/feed</link>";
						rss_xml += "<description>Moje powiadomienia</description>";
						auto all_notifications = notifications->get_all_notifications();
						for (auto& notification : all_notifications) {
							rss_xml += "<item>";
							rss_xml += "<title>" + notification.title + "</title>";
							rss_xml += "<link>http://100.76.115.82:10000/feed</link>";
							rss_xml += "<description>" + notification.message + "</description>";
							rss_xml += "<guid>" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(notification.timestamp.time_since_epoch()).count()) + "</guid>";
							rss_xml += "</item>";
						}
						rss_xml += "</channel>";
						rss_xml += "</rss>";

						response.body = rss_xml;
					}
					response.headers["Content-Length"] = std::to_string(response.body.size());
					return response;
				};
				rest.endpoints["feed"] = endpoint;
			}
			{ // /get_notifications
				std::shared_ptr<Endpoint> endpoint = std::make_shared<Endpoint>();
				endpoint->methods[HTTP::Method::GET] = [notifications](const HTTP& http) {
					if (!check_auth(http)) {
						HTTP response;
						response.type = HTTP::Type::RESPONSE;
						response.response.status = 401;
						response.response.reason = "Unauthorized";
						response.headers["Connection"] = "close";
						response.body = "401 Unauthorized\nWypad mi stad!";
						response.headers["Content-Length"] = std::to_string(response.body.size());
						return response;
					}

					HTTP response;
					response.type = HTTP::Type::RESPONSE;
					response.headers["Connection"] = "close";
					{
						nlohmann::json json = nlohmann::json::array();
						auto notifications_vec = notifications->get_all_notifications();
						for (auto& notification : notifications_vec) {
							json.push_back({{"title", notification.title}, {"message", notification.message}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(notification.timestamp.time_since_epoch()).count()}});
						}
						response.body = json.dump();
						response.headers["Content-Type"] = "application/json";
					}
					response.headers["Content-Length"] = std::to_string(response.body.size());
					return response;
				};
				endpoint->methods[HTTP::Method::POST] = [notifications](const HTTP& http) {
					if (!check_auth(http)) {
						HTTP response;
						response.type = HTTP::Type::RESPONSE;
						response.response.status = 401;
						response.response.reason = "Unauthorized";
						response.headers["Connection"] = "close";
						response.body = "401 Unauthorized\nWypad mi stad!";
						response.headers["Content-Length"] = std::to_string(response.body.size());
						return response;
					}

					HTTP response;
					response.type = HTTP::Type::RESPONSE;
					response.headers["Connection"] = "close";
					{
						nlohmann::json json = nlohmann::json::parse(http.body);
						std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::time_point{std::chrono::milliseconds((long long)json["timestamp"])};
						json = nlohmann::json::array();
						auto notifications_vec = notifications->get_notifications_after(timestamp);
						for (auto& notification : notifications_vec) {
							json.push_back({{"title", notification.title}, {"message", notification.message}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(notification.timestamp.time_since_epoch()).count()}});
						}
						response.body = json.dump();
						response.headers["Content-Type"] = "application/json";
					}
					response.headers["Content-Length"] = std::to_string(response.body.size());
					return response;
				};
				rest.endpoints["get_notifications"] = endpoint;
			}
			{ // POST /notification
				std::shared_ptr<Endpoint> endpoint = std::make_shared<Endpoint>();
				endpoint->methods[HTTP::Method::POST] = [notifications](const HTTP& http) {
					if (!check_auth(http)) {
						HTTP response;
						response.type = HTTP::Type::RESPONSE;
						response.response.status = 401;
						response.response.reason = "Unauthorized";
						response.headers["Connection"] = "close";
						response.body = "401 Unauthorized\nWypad mi stad!";
						response.headers["Content-Length"] = std::to_string(response.body.size());
						return response;
					}

					HTTP response;
					response.type = HTTP::Type::RESPONSE;
					response.headers["Connection"] = "close";
					{
						nlohmann::json json = nlohmann::json::parse(http.body);
						Notifications::Notification notification;
						notification.title = json["title"];
						notification.message = json["message"];
						notifications->add_notification(notification);
						response.body = "OK";
					}
					response.headers["Content-Length"] = std::to_string(response.body.size());
					return response;
				};
				rest.endpoints["notification"] = endpoint;
			}

			HTTP response = rest.call(http);
			std::string response_str = response.to_string();
			send(client_socket_fd, response_str.c_str(), response_str.size(), 0);
		}
		catch(std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	void server(std::shared_ptr<Notifications> notifications) {
		tcp_server server(std::bind(&connection_handler, std::placeholders::_1, notifications), 10000);
	}
}

namespace Clients {
	void connection_handler(int client_socket_fd, std::shared_ptr<Notifications> notifications) {
		try {
			auto prev_timestamp = std::chrono::system_clock::now();

			while(true) {
				notifications->wait_for_new_notification();
				auto new_timestamp = std::chrono::system_clock::now();

				auto notifications_vec = notifications->get_notifications_after(prev_timestamp);
				std::string notifications_str;
				for (auto& notification : notifications_vec) {
					notifications_str += notification.title + ";" + notification.message + ";" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(notification.timestamp.time_since_epoch()).count()) + "\n";
				}

				int ret = send(client_socket_fd, notifications_str.c_str(), notifications_str.size(), MSG_NOSIGNAL);
				if (ret < 0) {
					break;
				}

				prev_timestamp = new_timestamp;
			}

		}
		catch(std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	void server(std::shared_ptr<Notifications> notifications) {
		tcp_server server(std::bind(&connection_handler, std::placeholders::_1, notifications), 10001);
	}
}

int main() {
	std::shared_ptr<Notifications> notifications = std::make_shared<Notifications>();
	std::jthread API_thread(&API::server, notifications);
	std::jthread clients_thread(&Clients::server, notifications);
	API_thread.join();
	return 0;
}
