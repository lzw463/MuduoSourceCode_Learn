#pragma once

#include <functional>
#include <memory>

#include "Timestamp.h"
#include "noncopyable.h"
/*
Channel理解为通道，封装里了socket和其感兴趣的event，如EPOLLIN，EPOLLOUT事件
还绑定了poller返回的具体事件
*/


//头文件里只用到了类型的声明，没有用到类型的具体定义和方法，所以只需要前置声明，并不需要包含头文件
class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件（调用相应的回调函数）
    void handleEvent(Timestamp receiveTime);
    
    //设置回调函数对象
    //cb是一个左值，有名字有内存，出了函数之后，这个形参的局部对象就不需要了，用move将左值变为右值，资源直接给readcallback
    void setReadCallback(ReadEventCallback &cb) {readCallback_ = std::move(cb);} 
    void setWriteCallback(EventCallback &cb) {WriteCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback &cb) {CloseCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback &cb) {ErrorCallback_ = std::move(cb);}

    //防止channel被remove掉，channel还在执行回调操作
    //？？？？？？？？？？
    void tie(const std::shared_ptr<void)&)

    int fd() const {return fd_;}
    int events() const {return events_;}
    int set_revents(int revt) {revents_ = revt;}
    
    //让fd对读事件感兴趣,设置fd相应的事件状态
    //kReadEvent标志位，假设events初始值为0b0000，kReadEvents = 0b0001,读事件的标志位,events_  |= kread = 0b0001相当于设置来读事件标志位
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    //返回fd当前的事件状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}

    int index() {return index_;}
    void set_index(int idx) {index_ = idx;}

    EventLoop* ownerLoop() {return loop_;}
    void remove();
private:

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; //事件循环
    const int fd_;    //fd,poller监听的fd
    int events_;      //注册fd感兴趣的事件
    int revents_;     //POLLER返回的实际发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    //因为channel能够获知fd最终发生的revents，所以他负责调用具体时间的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    void update();
    void hanleEventWithGuard(Timestamp* reveiveTime);
};


/*
EventLoop包含了channel和poller
channel包含了fd和感兴趣的事件以及最终发生的事件
这些事件向poller里面注册
发生了事件由poller通知channel
channel得到相应的事件通知后调用预置的回调函数

对应多路事件分发器？？？？？？？？？？？？？
*/