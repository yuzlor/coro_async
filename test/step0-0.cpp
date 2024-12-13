#include <chrono>
#include <coroutine>
#include <iostream>

struct RepeatAwaiter
{
    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine) const noexcept
    {
        if (coroutine.done())
            return std::noop_coroutine();
        else
            return coroutine;
    }

    void await_resume() const noexcept {}
};

struct Promise
{
    auto initial_suspend()
    {
        return std::suspend_always();
    }

    auto final_suspend() noexcept
    {
        return std::suspend_always();
    }

    void unhandled_exception()
    {
        throw;
    }

    auto yield_value(int ret)
    {
        mRetValue = ret;
        return RepeatAwaiter();
    }

    // void return_void()
    // {
    //     mRetValue = 0;
    // }

    std::coroutine_handle<Promise> get_return_object()
    {
        return std::coroutine_handle<Promise>::from_promise(*this);
    }

    int mRetValue;
};

struct Task
{
    using promise_type = Promise;

    Task(std::coroutine_handle<promise_type> coroutine)
        : mCoroutine(coroutine) {}

    std::coroutine_handle<promise_type> mCoroutine;
};

Task hello()
{
    std::cout << "hello 42" << std::endl;
    co_yield 42;
    std::cout << "hello 12" << std::endl;
    co_yield 12;
    std::cout << "hello 6" << std::endl;
    co_yield 6;
    std::cout << "hello 结束" << std::endl;
    // co_return;
}

int main()
{
    std::cout << "main即将调用hello" << std::endl;
    Task t = hello();
    std::cout << "main调用完了hello" << std::endl;
    while (!t.mCoroutine.done())
    {
        t.mCoroutine.resume();
        std::cout << "main得到hello结果为" << t.mCoroutine.promise().mRetValue;
    }
    return 0;
}
