#include "co_async/debug.hpp"

#include <system_error>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace co_async
{
    auto checkError(auto res)
    {
        // 把C语言错误码转换为C++异常
        if (res == -1) [[unlikely]]
        {
            throw std::system_error(errno, std::system_category());
        }
        return res;
    }

}