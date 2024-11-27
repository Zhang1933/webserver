#include "muduo/net/Buffer.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Types.h"
#include "muduo/net/SocketsOps.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>

using namespace muduo;

const char Buffer::kCRLF[] = "\r\n";


ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    ssize_t n=0;
    while(true){
        const size_t writable=writableBytes();
        vec[0].iov_base=begin()+writerIndex_;
        vec[0].iov_len=writable;
        vec[1].iov_base=extrabuf;
        vec[1].iov_len=sizeof(extrabuf);
        // when there is enough space in this buffer, don't read into extrabuf.
        // when extrabuf is used, we read 128k-1 bytes at most.
        const int iovcnt=(writable<sizeof(extrabuf))?2:1;
        n=sockets::readv(fd,vec, iovcnt);
        if (n < 0)
        {
            *savedErrno = errno;
            break;
        }
        else if(n==0){
            break;
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
        }
    }
    return n;
}