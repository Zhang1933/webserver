#include "muduo/net/Buffer.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Types.h"
#include "muduo/net/SocketsOps.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>

using namespace muduo;


ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
reread:
    const size_t writable=writableBytes();
    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writable;
    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof(extrabuf);
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt=(writable<sizeof(extrabuf))?2:1;
    const ssize_t n=sockets::readv(fd,vec, iovcnt);
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable)
    {
        writerIndex_ += static_cast<size_t>(n);
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, static_cast<size_t>(n) - writable);
    }
    if (implicit_cast<size_t>(n) == writable + sizeof extrabuf)
    {
        LOG_INFO<<"message to large,reread buffer";
        goto reread;
    }
    return n;
}