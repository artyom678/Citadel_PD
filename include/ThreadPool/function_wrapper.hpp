#pragma once

#include <functional>
#include <memory>

template<typename Ret, typename... Args>
class function_wrapper;


template<typename Ret, typename... Args>
class function_wrapper<Ret(Args...)> {


	struct BaseFunc {

		virtual ~BaseFunc() = default;

		virtual Ret call(Args&&...) = 0;

	};

	template<typename F>
	struct DerivedFunc : BaseFunc {


		DerivedFunc(F&& f) 
			: f(std::move(f)) 
		{}


		Ret call(Args&&... args) override {

			return std::invoke(f, std::forward<Args>(args)...);
		}

	private:
		F f;
	};


public:

	function_wrapper() = default;

	template<typename F>
	function_wrapper(F f) 
		: func_ptr(new DerivedFunc(std::move(f)))
	{}

	function_wrapper(function_wrapper&& other) noexcept
		: func_ptr(std::move(other.func_ptr))
	{}

	function_wrapper& operator=(function_wrapper&& other) noexcept {

		if (&other != this) {
			
			func_ptr = std::move(other.func_ptr);			
		}

		return *this;
	}

	template<typename F>
	function_wrapper& operator=(F f) & {
		
		func_ptr.reset(new DerivedFunc(std::move(f)));
		return *this;
	}


	function_wrapper(const function_wrapper&) = delete;

	function_wrapper& operator=(const function_wrapper&) = delete;

	Ret operator()(Args&&... args) {

		if (!func_ptr) {
			throw std::bad_function_call();
		}
		return func_ptr->call(std::forward<Args>(args)...);
	}

private:
	std::unique_ptr<BaseFunc> func_ptr;
};