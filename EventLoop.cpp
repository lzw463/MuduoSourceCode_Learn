#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

//防止一个线程创建多个EventLoop
//__thread的作用：虽然这个变量是全局变量，但是加上__thread就会在每个线程中有独立实体，不会相互干扰
__thread EventLoop *t_loopInThisThread = nullptr;


//定义默认的Poller IO复用的超时时间
const int kPollTimeMs = 10000;

//创建wakeupfd用来notify唤醒subreactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another Eventloop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //设置wakeupfd的事件类型以及发生时间的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();

}

//每个reactor有一个wakeupfd，mainreactor可以通过向subreactor write一个消息使subreactor感知到


EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %zd bytes instead of 8", n);
    }
}


void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            //poller监听哪些channel发生事件了，然后上报给Eventloop,通知channel处理事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前Eentloop事件循环需要处理的回调操作
        /*
        IO线程：mainloop 主要accept 会返回专门跟客户端通信的fd，用channel打包fd唤醒某一个subloop
        唤醒之后channel里面现在只有一个wakeupfd
        mainloop事先注册一个回调cb（需要subloop来执行）  wakeup subloop之后，执行下面的方法，执行之前mainloop注册的回调
        */
        doPendingFunctors();
        
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

//退出时间循环 1：loop在自己的线程中调用quit 2：在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopTheread()) //如果是在其他线程中调用quit，在subloop中调用了mainloop的quit函数
    {
        wakeup(); //唤醒以后，subloop才会从阻塞的poll中返回
    }
}
//muduo没有采用工作队列的方式：mainloop生产channel，subloop消费channel


void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopTheread()) //在当前的loop线程中，执行cb
    {
        cb();
    }
    else //在非当前线程中执行，需要唤醒loop所在线程执行cb
    {
        queueInLoop(cb);
    }
}


void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);  //直接构造而不是拷贝构造
    }

    //唤醒相应的，需要执行上面回调操作的loop线程
    //calling:当前loop正在执行回调，但是loop又有了新的回调
    if(!isInLoopTheread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

//唤醒loop所在的线程,向wakeupfd写一个数据
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

//调用poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    
    }

    for (const Functor &functor : functors)
    {
        functor(); //执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}


/*
        mainloop

                                no=================生产者-消费者的线程安全的队列

subloop1   subloop2   subloop2
*/