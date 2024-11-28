#pragma once

#include "muduo/base/Logging.h"
#include "muduo/base/copyable.h"
#include <cstddef>
#include <cstdio>
#include <map>
#include <mutex>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class File{
public:
    File(std::string p,ssize_t sz)
    {
        this->fd=::open(p.data(), O_RDONLY);
        if(fd==-1)
        {
            LOG_SYSERR<<"open file failied"<<  strerror(errno) ;
        }
        filesize=sz;
    }
    File()
    {
        filesize=-1;
        fd=-1;
    }
   
    ~File() // FIXME:用shared_ptr 管理文件
    {
        // if(fd!=-1){
        //     ::close(fd);
        // }
    }
public:
    ssize_t filesize=0;
    int fd=-1;
};

class FileSingleton{
public:
    ~FileSingleton()=default;
    FileSingleton(const FileSingleton&)=delete;
    static FileSingleton& get_instance(){
        static FileSingleton instance;
        return instance;
    }

    ssize_t getFileSize(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it=Filesmp_.find(name);
        if(it==Filesmp_.end())
        {
            return -1;
        }
       return it->second.filesize;
    }
    int getFileDes(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it=Filesmp_.find(name);
        if(it==Filesmp_.end())
        {
            return -1;
        }
       return it->second.fd;
    }
    void insertFile(const std::string& name,const ssize_t& sz)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it=Filesmp_.find(name);
        if(it!=Filesmp_.end())
        {
            return;
        }
        Filesmp_[name]=File(name,sz);
    }
private:
    FileSingleton()=default;
    std::map<std::string,File> Filesmp_;
    std::mutex mutex_;
};

