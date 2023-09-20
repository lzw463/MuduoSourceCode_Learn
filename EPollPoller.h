#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/*
epoll的使用
epoll_create
epoll_ctl add/mod/del
epoll_wait
*/

class EPolllPoller : public Poller
{
public:
    EPolllPoller(EventLoop *loop);
    ~EPolllPoller() override;
    
    //重写基类Poller的抽象方法
    Timestamp poll(int timeoutMS, ChannelList *activateChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kIninEventListSize = 16;

    //填写活跃的连接
    void fillActivateChannels(int numEvents, ChannelList *activateChannelLists) const;
    //更新channel通道
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};