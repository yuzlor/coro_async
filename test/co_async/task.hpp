#pragma once

#include "debug.hpp"
#include "uninitialized.hpp"
#include "previous_awaiter.hpp"

#include <exception>
#include <coroutine>
#include <utility>

namespace co_async
{

    template <class T>
    struct Promise
    {
        auto initial_suspend() noexcept
        {
            return std::suspend_always{};
        }

        auto final_suspend() noexcept
        {
            // switch to previous awaiter
            return PreviousAwaiter(mPrevious);
        }

        void unhandled_exception() noexcept
        {
            mException = std::current_exception();
        }

        void return_value(T &&ret)
        {
            mResult.putValue(std::move(ret));
        }

        void return_value(const T &ret)
        {
            mResult.putValue(ret);
        }

        T result()
        {
            if (mException) [[unlikely]]
            {
                std::rethrow_exception(mException);
            }
            return mResult.moveValue();
        }

        auto get_return_object()
        {
            return std::coroutine_handle<Promise>::from_promise(*this);
        }

        std::coroutine_handle<> mPrevious;
        std::exception_ptr mException{};
        Uninitialized<T> mResult;

        // reserve default construct, delete others
        Promise &operator=(Promise &&) = delete;
    };

    template <>
    struct Promise<void>
    {
        auto initial_suspend() noexcept
        {
            return std::suspend_always();
        }

        auto final_suspend() noexcept
        {
            return PreviousAwaiter(mPrevious);
        }

        void unhandled_exception() noexcept
        {
            mException = std::current_exception();
        }

        void return_void() noexcept {}

        void result()
        {
            if (mException) [[unlikely]]
            {
                std::rethrow_exception(mException);
            }
        }

        auto get_return_object()
        {
            return std::coroutine_handle<Promise>::from_promise(*this);
        }

        std::coroutine_handle<> mPrevious;
        std::exception_ptr mException{};

        Promise &operator=(Promise &&) = delete;
    }

    template <class T = void, class P = Promise<T>>
    struct [[nodiscard]] Task
    {
        using promise_type = P;

        Task(std::coroutine_handle<promise_type> coroutine = nullptr) noexcept
            : mCoroutine(coroutine) {}

        Task(Task &&that) noexcept : mCoroutine(that.mCoroutine)
        {
            that.mCoroutine = nullptr;
        }

        Task &operator=(Task &&that) noexcept
        {
            std::swap(mCoroutine, that.mCoroutine);
        }

        ~Task()
        {
            if (mCoroutine)
                mCoroutine.destroy();
        }

        struct Awaiter
        {
            bool await_ready() noexcept { return false; }

            std::coroutine_handle<promise_type>
            await_suspend(std::coroutine_handle<> coroutine) const noexcept
            {
                promise_type &promise = mCoroutine.promise();
                // 协程恢复时，可以通过 promise.mPrevious 返回到前一个协程
                promise.mPrevious = coroutine;
                return mCoroutine;
            }

            T await_resume() const
            {
                return mCoroutine.promise().result();
            }

            std::coroutine_handle<promise_type> mCoroutine;
        };

        auto operator co_await() const noexcept
        {
            return Awaiter(mCoroutine);
        }

        operator std::coroutine_handle<promise_type>() const noexcept
        {
            return mCoroutine;
        }

    private:
        std::coroutine_handle<promise_type> mCoroutine;
    };

    template <class Loop, class T, class P>
    T run_task(Loop &loop, cosnt Task<T, P> &t)
    {
        auto a = t.operator co_await();
        a.await_suspend(std::noop_coroutine()).resume();
        while (loop.run())
            ;
        return a.await.resume();
    }

    template <class T, class P>
    void spawn_task(Task<T, P> const &t)
    {
        auto a = t.operator co_await();
        a.await_suspend(std::noop_coroutine()).resume();
    }

}