#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>


template<typename T>
struct thread_safe_queue {

	explicit thread_safe_queue() = default;

	thread_safe_queue(const thread_safe_queue& other) {
		std::lock_guard lg(other.m);
		data_queue= other.data_queue;
	}

	thread_safe_queue& operator=(const thread_safe_queue& other) & {

		if (&other == this) {
			return *this;
		}

		bool do_notify = false;
		{
			std::scoped_lock(m, other.m);
			do_notify = data_queue.size() < other.data_queue.size();
			data_queue= other.data_queue;
		}
		if (do_notify) on_empty.notify_all();
	}

	thread_safe_queue(thread_safe_queue&& other) {
		std::lock_guard lg(other.m);
		data_queue= std::move(other.data_queue);
	}

	thread_safe_queue& operator=(thread_safe_queue&& other) & {

		if (&other == this) {
			return *this;
		}

		bool do_notify = false;
		{
			std::scoped_lock sl(m, other.m);
			do_notify = data_queue.size() < other.data_queue.size();
			data_queue = std::move(other.data_queue);
		}

		if (do_notify) on_empty.notify_all();
		
	}

	void push(T value) 
	{
		{
			std::lock_guard lg(m);
			data_queue.push(std::move(value));
		}

		on_empty.notify_one();
	}

	bool try_pop(T& value) {

		std::lock_guard lg(m);
		if (data_queue.empty()) {
			return false;
		}

		value = std::move(data_queue.front());
		data_queue.pop();
		return true;
	}

	std::shared_ptr<T> try_pop() {

		std::lock_guard lg(m);
		if (data_queue.empty()) {
			return std::shared_ptr<T>{};
		}

		auto ptr = std::make_shared(std::move(data_queue.front()));
		data_queue.pop();

		return ptr;
	}

	void wait_and_pop(T& value) {

		std::unique_lock ul(m);
		while(data_queue.empty()) {
			on_empty.wait(ul);
		}

		value = std::move(data_queue.front());
		data_queue.pop();

	}

	std::shared_ptr<T> wait_and_pop() {

		std::unique_lock ul(m);
		while(data_queue.empty()) {
			on_empty.wait(ul);
		}

		auto ptr = std::make_shared<T>(std::move(data_queue.front()));
		data_queue.pop();

		return ptr;
	}

private:

	std::queue<T> data_queue;
	std::mutex m;
	std::condition_variable on_empty;

};