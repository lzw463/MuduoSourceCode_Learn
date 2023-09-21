#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_ = 0;

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();

}
Thread::~Thread()
{
    if (started_ && !joined_) //detach把线程设置为一个分离线程也就是守护线程，当主线程结束这些线程会自动结束，detach和join不能同时执行，join就是一个普通工作线程
    {
        thread_->detach(); //thread类提供的设置分离线程的方法；
    }
}


//假如start线程走的快，到semwait，子线程开启后还未走到sempost，主线程就要等待sempost，此时一定走完tid
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    //开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        //获取线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem); //信号量加1
        func_(); //开启一个新线程，专门执行该线程函数，肯定包含一个eventloop
    }));

    //这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);

}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}