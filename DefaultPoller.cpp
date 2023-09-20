#include "Poller.h"
#include <stdlib.h> //获取环境变量


Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return  nullptr; //生成poll的实例
    }
    else
    {
        return new EPollPoller(loop); //生成epoll的实例
    }
}