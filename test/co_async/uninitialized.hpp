#pragma once

#include "non_void_helper.hpp"

#include <utility>
#include <memory>

namespace co_async
{
    // 用于在不初始化的情况下存储一个类型的对象，并在需要时进行初始化和移动
    template <class T>
    struct Uninitialized
    {
        // 联合体允许在同一内存位置存储不同类型的数据，但只能同时存储其中一个成员
        // 这里使用联合体是为了在不初始化的情况下占用内存
        union
        {
            T mValue;
        };

        Uninitialized() noexcept {}

        Uninitialized(Uninitialized &&) = delete;

        ~Uninitialized() noexcept {}

        T moveValue()
        {
            T ret(std::move(mValue));
            mValue.~T();
            return ret;
        }

        template <class... Ts>
        void putValue(Ts &&...args)
        {
            new (std::addressof(mValue)) T(std::forward<Ts>(args)...);
        }
    };

    template <>
    struct Uninitialized<void>
    {
        auto moveValue()
        {
            return NonVoidHelper<>{};
        }

        void putValue(NonVoidHelper<>) {}
    };

    template <class T>
    struct Uninitialized<T const> : Uninitialized<T>
    {
    };

    template <class T>
    struct Uninitialized<T &> : Uninitialized<std::reference_wrapper<T>>
    {
    };

    template <class T>
    struct Uninitialized<T &&> : Uninitialized<T>
    {
    };

} // namespace co_async
