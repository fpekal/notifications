#pragma once
#include <chrono>
#include <vector>
#include <mutex>

class Notifications {
public:
	struct Notification {
		std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
		std::string title;
		std::string message;
	};

	Notifications() {
		removal_thread = std::jthread(&Notifications::removal_thread_func, this);
	}

	std::vector<Notification> get_all_notifications() {
		std::lock_guard<std::mutex> lock(notifications_mutex);
		return notifications;
	}

	std::vector<Notification> get_notifications_after(const std::chrono::time_point<std::chrono::system_clock>& timestamp) {
		std::lock_guard<std::mutex> lock(notifications_mutex);
		std::vector<Notification> result;
		for (auto& notification : notifications) {
			if (notification.timestamp > timestamp) {
				result.push_back(notification);
			}
		}
		return result;
	}

	void add_notification(const Notification& notification) {
		std::lock_guard<std::mutex> lock(notifications_mutex);
		notifications_count++;
		notifications.push_back(notification);
		new_notification_cv.notify_all();
	}

	void remove_old_notifications() {
		std::lock_guard<std::mutex> lock(notifications_mutex);
		notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [this](const Notification& notification) {
			return notification.timestamp < std::chrono::system_clock::now() - removal_time;
		}), notifications.end());
	}

	void removal_thread_func() {
		while (true) {
			remove_old_notifications();
			std::this_thread::sleep_for(std::chrono::seconds(60));
		}
	}

	long long get_notification_count() {
		std::lock_guard<std::mutex> lock(notifications_mutex);
		return notifications_count;
	}

	void wait_for_new_notification() {
		long long prev_notification_count = get_notification_count();
		std::unique_lock<std::mutex> lock(notifications_mutex);
		while (notifications_count == prev_notification_count) {
			new_notification_cv.wait(lock);
		}
	}

	std::chrono::seconds removal_time = std::chrono::hours(24*2);


private:
	long long notifications_count = 0;

	std::condition_variable new_notification_cv;
	std::mutex notifications_mutex;
	std::jthread removal_thread;
	std::vector<Notification> notifications;
};
