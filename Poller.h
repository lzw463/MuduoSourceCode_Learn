#pragma once

#include <vector>
#include <unordered_map>
#include "noncopyable.h"


class Channel;
class EventLoop;

//muduo库中多路事件分发器的核心IO复用模块
class Poller
{
public:
    using ChannelList = std::vector<Channel*>;


protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_; //能被派生对象访问

private:
    EventLoop *ownerloop; //定义poller所属的事件循环eventloop
     

};