#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"

#include <errno.h>

//channel未添加到poller中
const int kNew = -1; //channel成员函数index = -1 
//channel已添加到poller中
const int kAdded = 1;
//channel从poller中删除
const int kDeleted = 2;



/*
            Eventloop
    channellist      Poller
                    ChannelMap <fd, channel*>

*/

EPolllPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) //epoll_create
    , event_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno)
    }
}

EPolllPoller::~EPolllPoller()
{
    ::close(epollfd_);
}

//
void EPolllPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd=%d events=%d index=%d \n",__FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channel_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else  //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNonEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            Channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EPolllPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func = %s => fd=%d",__FUNCTION__, channel->fd());

    int index = channel->index();
    if (index = kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }

}

void EPolllPoller::fillActivateChannels(int numEvents, ChannelList *activateChannelLists) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); //EventLoop就拿到poller给他返回的所有发生事件的channel
    }

}

//更新channel通道，epoll_ctl add/del/mod
void EPolllPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    int fd = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl_del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl_add/mod error:%d\n", errno);
        }
    }
}

Timestamp EPolllPoller::poll(int timeoutMS, ChannelList *activeChannels)
{
    //poll调用非常频繁，loginfo每次都会打印，影响效率，所以应设置为logdebug
    LOG_INFO("func=%s => fd total count:%d\n", channels_.size());
    
    int numEvents = ::epoll_wait(epollfd_, *events_.begin(), static_cast<int>events_.size(), tomeoutMS);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d, events happened \n", numEvents);
        fillActivateChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2));
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__)
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}