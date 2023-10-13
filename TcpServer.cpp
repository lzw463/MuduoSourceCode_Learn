#include "TcpServer.h"
#include "Logger.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"

#include <functional>
#include <string.h>
    
EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option)
                : loop_(CheckLoopNotNull(loop))
                , ipPort_(listenAddr.toIpPort())
                , name_(nameArg)
                , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
                , threadPool_(new EventLoopThreadPool(loop, name_))
                , connectionCallback_()
                , messageCallback_()
                , nextConnId_(1)
                , started_(0)
{
    //当有新用户连接时，会执行Tcpserver::newconnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));

} 

TcpServer::~TcpServer()
{
    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset(); //connection的智能指针释放掉，不再用强智能指针去管理资源
        //销毁连接
        conn->getloop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        ); 
    }
}

//有一个新的客户端的连接，会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)//轮询
{
    //轮询算法选择一个subloop，来管理channel
    EventLoop *ioloop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection[%s] - new connection [%s] from %s \n",
            name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    //通过sockfd获取其绑定的本地ip地址和端口号
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    //根据连成功的sockfd，创建Tcpconnection对象
    TcpConnectionPtr conn(new TcpConnection(
                            ioloop,
                            connName,
                            sockfd, //socket channel
                            localAddr,
                            peerAddr));

    connections_[connName] = conn;

    //下面的回调都是用户设置给Tcpserver的=》Tcpconnection=》channel=》poller=》notify channel
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback_(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    //设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );
    //直接调用
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_++ == 0) //防止一个对象被start多次
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
} 

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
        name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop *ioloop = conn->getloop();
    ioloop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}