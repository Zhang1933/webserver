#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include "muduo/base/copyable.h"
#include <assert.h>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>


namespace muduo
{

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer :muduo::copyable
{
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;
  Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
  {
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
  }

  // default copy-ctor, dtor and assignment are fine
  void swap(Buffer& rhs)
  {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }


    size_t readableBytes() const
    { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const
    { return readerIndex_; }

    // peek 读到哪了
    const char* peek() const
    { return begin() + readerIndex_; }

  const char* findCRLF() const
  {
    // FIXME: replace with memmem()?
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        readerIndex_ += len;
    }

    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(static_cast<size_t>(end - peek()));
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 默认retrieve all string
    std::string retrieveAsString()
    {
        std::string str(peek(), readableBytes());
        retrieveAll();
        return str;
    }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void append(const std::string& str)
    {
        if(str.empty())return;
        append(str.data(), str.length());
    }
    void append(const char* /*restrict*/ data, size_t len)
    {
        assert(len>0);
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len);
    }
    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }
  
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char* beginWrite()
    { return begin() + writerIndex_; }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    void hasWritten(size_t len)
    { writerIndex_ += len; }

    //从readerIndex_往前添加
    void prepend(const void* /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    // shrink writable size
    void shrink(size_t reserve)
    {
        std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
        std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
        buf.swap(buffer_);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t readFd(int fd, int* savedErrno);

private:

    char* begin()
    { return &*buffer_.begin(); }

    const char* begin() const
    { return &*buffer_.begin(); }

    // 创建可写的len空间
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_,
                        begin()+writerIndex_,
                        begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    static const char kCRLF[];
};

}

#endif