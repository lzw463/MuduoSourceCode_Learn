#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>

//从fd上读取数据：poller工作在LT模式、
//buffer缓冲区是有大小的，但是从fd上读数据的时候，却不知道tcp数据最终的大小
//开的特别大会浪费，所以先用两个缓冲区，一个是缓冲区剩余的可写空间，一个是栈上的临时空间
//最后将栈上的数据写到缓冲区后面，不会造成一点浪费
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65535] = {0}; //栈上的内存空间 64k
    struct iovec vec[2];
    const size_t writeable = writeableBytes(); //这是剩余的可写空间
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    
    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const size_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writeable) //buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else //extrabuf里面也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeable); //writerIndex_开始写到buffer
    }
    return n;
    
}
//readv()可以在非连续的多块缓冲区里写入同一个fd上读出来的数据
