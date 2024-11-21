#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Thread.h"
#include "muduo/net/db/redis.h"
#include <cstdio>

using namespace muduo;

class DerivedSingle:public DbSingleton<DerivedSingle>{
   // !!!! attention!!!
   // needs to be friend in order to
   // access the private constructor/destructor
   friend class DbSingleton<DerivedSingle>;
public:
   DerivedSingle(const DerivedSingle&)=delete;
   DerivedSingle& operator =(const DerivedSingle&)= delete;
private:
   DerivedSingle()=default;
};


class Test :public DbSingleton<Test>
{
public:
  const string& name() const { return name_; }
  void setName(const string& n) { name_ = n; }
    Test(token)
    {
        printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
    }
    ~Test()
    {
        printf("tid=%d, desstructing %p,name:%s\n", muduo::CurrentThread::tid(), this,this->name_.c_str());
    }
private:
  string name_;
};

void threadFunc()
{
  printf("tid=%d, %p name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::DbSingleton<Test>::get_instance(),
         muduo::DbSingleton<Test>::get_instance().name().c_str());
  muduo::DbSingleton<Test>::get_instance().setName("only one, changed");
}

int main()
{
  // TEST Singleton
  muduo::DbSingleton<Test>::get_instance().setName("only one");
  
  muduo::Thread t1(threadFunc);
  t1.start();
  t1.join();
  printf("tid=%d, %p name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::DbSingleton<Test>::get_instance(),
         muduo::DbSingleton<Test>::get_instance().name().c_str());

}
