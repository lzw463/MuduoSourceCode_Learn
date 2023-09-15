#pragma once


#include "noncopyable.h"
#include "Timestamp.h"
#include "Eventloop.h"

#include <vector>
#include <unordered_map>


class Channel;

//muduo库中多路事件分发器的核心IO复用模块
class Poller :: noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeouts, ChannelList *activeChannels) = 0;

    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
;
    //判断参数channel是否在当前Poller中
    bool hasChannel(Channel *channel) const;

    //Eventloop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    //
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_; //能被派生类成员访问

private:
    EventLoop *ownerloop; //定义poller所属的事件循环eventloop
     

};