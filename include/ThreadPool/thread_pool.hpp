#pragma once

#include "thread_safe_queue.hpp"
#include <functional>
#include <thread>
#include <atomic>
#include <type_traits>
#include <vector>
#include <future>
#include "function_wrapper.hpp"

struct StaticThreadPool {

	using Task = function_wrapper<void()>;


	explicit StaticThreadPool(std::size_t count_threads)
		: is_active{true}
	{


		for(std::size_t i = 0; i != count_threads; ++i) {

			try {
				workers.emplace_back([this]{

					while(is_active.load()) {
						
						Task task;
						tasks.wait_and_pop(task);

						task();
					}

				});
			} catch(...) {

				is_active.store(false);
				for (std::size_t j = 0; j != i; ++j) {

					tasks.push([]{});
				}

				for(std::size_t j = 0; j != i; ++j) {
					workers[j].join();
				}

				throw;

			}
		}
	}

	StaticThreadPool(const StaticThreadPool&) = delete;

	StaticThreadPool& operator=(const StaticThreadPool&) = delete;


	template<typename F, typename... Args>
	auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
	{
		
		using Ret = std::invoke_result_t<F, Args...>;

		auto bound_task = std::bind([](auto&& f, auto&&... args){ return std::invoke(f, args...); }
			, std::forward<F>(f), std::forward<Args>(args)...);
		
        std::packaged_task<Ret()> task(std::move(bound_task));
		auto result = task.get_future();
		tasks.push(std::move(task));

		return result;
	}



	~StaticThreadPool() {

		is_active.store(false);

		for(std::size_t i = 0; i != workers.size(); ++i) {
			tasks.push([]{});
		}

		for(auto& w : workers) {
			w.join();
		}

	}


private:

	std::vector<std::thread> workers;
	thread_safe_queue<Task> tasks;
	std::atomic_bool is_active;

};