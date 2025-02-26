# webserver

基于[chenshuo/muduo](https://github.com/chenshuo/muduo)

与作者的webserver相比，新添功能&特性：
* 支持踢掉空闲连接
* 支持web视频播放、图片展示，使用内存与文件大小无关，只与连接数有关。
* 支持用户登录、注册，提供 Redis API。Redis API 使用 Redis 连接池提供 session 功能，有懒连接功能。（调用了一个Redis API库）
* epoll使用边缘触发，压力测试下发现QPS比水平触发提升了约25%。

代码的更改都在muduo/net目录下，muduo/base下的代码没有更改。

## webserver测试：

使用[webbench-1.5](https://github.com/qinguoyi/TinyWebServer/tree/master/test_pressure/webbench-1.5)。

同一台电脑.与[youshuang](https://github.com/qinguoyi/TinyWebServer)的进行比较

* 内存：16G
* CPU: [AMD Ryzen™ 9 7945HX](https://www.amd.com/en/products/processors/laptop/ryzen/7000-series/amd-ryzen-9-7945hx.html)，16核，32线程
* ubuntu22.04 物理机环境
* 线程池大小：16

* 本webserver:

![alt text](images/thismuduo.png)

支持每秒9w的请求。

* youshuang

![alt text](images/youshuang.png)

* 原版muduo粗略比较

粗略比较，比较并不公平，因为muduo主页返回的是内存中写死的内容，不用进行文件读写。

```
$ ./webbench -c 10500 -t 5 --get -1  http://127.0.0.1:8000/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/
10500 clients, running 5 sec.

Speed=4451076 pages/min, 14094922 bytes/sec.
Requests: 370923 susceed, 0 failed.
```

添加的功能并没有降低QPS，吞吐量更大。


## FAQ:

1. 讲一下accept连接的流程

   listening socket 可读后，调用Accepter的handleRead,Accepter::handleRead会一直accept新连接，每accept到一个新连接，调用TcpServer的TcpServer::newConnection，TcpServer::newConnection获取线程池中的下一个subreactor，创建TcpConnection对象，设置HttpServer::onMessage的messageCallback，设置HttpServer::onConnection的connectionCallback_，设置HttpServer::onWriteComplete writeCompleteCallback，让subreactoer负责处理connection的消息。
   创建connection对象的时候，构造函数传入获取的事件循环对象指针，在子事件循环中注册调用TcpConnection::connectEstablished函数，TcpConnection::connectEstablished会将套接字加入子事件循环的监听列表中，开始监听套接字上的消息。

   * TcpServer对象维护一个shared_ptr，防止用的时候，connection对象被销毁了。

2. 往一个远程断开连接的套接字写或者读会返回什么？
   * 会触发一个SIGPIPE 信号，信号可以终止进程，需要设置把他忽略调。函数返回-1，并设置EPIPE错误。
   * 如果远程主机正常关闭连接，读操作会返回 0，表示连接已关闭。
3. 非阻塞模式下，读tcp套接字，没有消息返回什么?
   如果没有数据可读，recv 或 read 会返回 -1。errno 会被设置为 EAGAIN 或 EWOULDBLOCK。
4. 非阻塞模式下，写tcp套接字，buffer满了会返回什么？
   会返回-1并设置,EAGAIN 或 EWOULDBLOCK。如果写buffer的中途满了，会返回实际写的数量。

5. 文件传输使用内存与文件大小无关只与连接数有关是怎么做到的
   * 在服务器返回消息时，先调用connection send函数向connection发送响应头中的消息，发送完后将HttpServer::onWriteComplete回调函数注册到事件循环的任务队列。HttpServer::onWriteComplete判断是否有文件要返回并且没发送完，如果有，读取64kB文件内容，调用connection send函数发送，这样一直循环直到文件读完。
   * 会先写，写完如果满了，就监听可写事件写。


7. 踢掉空闲连接是怎样做的？
   这里用链表管理与维护的。连接建立与关闭都会调用 HttpServer::onConnection。
   每个eventloop，也就是sub reactor中有一个链表。连接创建时，向链表尾添加连接指针，连接记录在链表中的位置。

   每个sub eventloop 每隔一秒检查链表中是否有超过一定时间的未活跃连接，如果有，会先关闭server的写端。然后放到subreactor事件循环中的任务队列中关闭连接，链表除以连接。

   连接不是超时关闭时，获取连接记录的链表位置，链表中移除连接。

6. 用户登录是怎样做的？
  数据库连接还没有实现。
  从request对象中回去请求体内容，解析出用户名与密码，设置cookies字段返回给用户。

7. http请求是怎么解析的？
   connection中有消息来，会读入到buffer中去，回调函数调用HttpServer::onMessage，connection中会记录当前的http context，context开始读buffer内容，初始化为请求行状态，开始解析请求行，请求行解析完，进入期待请求头状态开始解析请求头，如果没有冒号只有回车换行符表明请求头解析完毕，请求头解析完毕后进入请求体解析状态，将结构放到context的request对象中里面。

8. 返回图片，视频，使用内存与文件无关是怎么做的？
   * 每次从文件里面读64k到buffer，tcp连接会记录读的偏移，buffer往套接字里写，写完调用注册的回调函数，回调函数会接着读下一个64k，这么一直循环读文件，写套接字，回调，直到把文件读完。pread返回读的字节数，如果pread返回0，就表示读完了，可以不用发数据了。


# TODO:

* 性能调优

```bash
Samples: 3K of event 'cycles:P', Event count (approx.): 104877547589
  Children      Self  Command          Shared Object         Symbol
+   21.04%     0.00%  httpserver_test  [unknown]             [k] 0xffffffffffffffff
+   14.76%     0.76%  httpserver_test  [kernel.kallsyms]     [k] do_syscall_64
+   14.47%     0.09%  httpserver_test  [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+   14.04%     0.13%  httpserver_test  [kernel.kallsyms]     [k] x64_sys_call
+    7.57%     0.00%  httpserver_test  httpserver_test       [.] muduo::Acceptor::handleRead()
+    7.29%     0.04%  httpserver_test  httpserver_test       [.] muduo::TcpServer::newConnection(int, muduo::InetAddress const&)
+    5.95%     0.00%  httpserver_test  httpserver_test       [.] muduo::EventLoop::loop()
+    5.31%     0.06%  httpserver_test  libc.so.6             [.] accept4
+    5.30%     0.00%  httpserver_test  httpserver_test       [.] muduo::Socket::accept(muduo::InetAddress*)
+    5.26%     0.00%  httpserver_test  httpserver_test       [.] muduo::sockets::accept(int, sockaddr_in*)
+    5.24%     0.00%  Thread4          [unknown]             [k] 0xffffffffffffffff
+    5.23%     0.00%  Thread3          [unknown]             [k] 0xffffffffffffffff
+    5.16%     0.00%  Thread8          [unknown]             [k] 0xffffffffffffffff
+    5.16%     0.00%  Thread1          [unknown]             [k] 0xffffffffffffffff
+    5.09%     0.09%  httpserver_test  [kernel.kallsyms]     [k] __sys_accept4
+    5.05%     0.00%  httpserver_test  [kernel.kallsyms]     [k] __x64_sys_accept4
+    4.91%     0.04%  httpserver_test  [kernel.kallsyms]     [k] do_accept
+    4.88%     0.00%  Thread2          [unknown]             [k] 0xffffffffffffffff
+    4.63%     0.00%  Thread4          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.63%     0.00%  Thread4          [kernel.kallsyms]     [k] do_syscall_64
+    4.60%     0.00%  Thread3          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.60%     0.00%  Thread3          [kernel.kallsyms]     [k] do_syscall_64
+    4.57%     0.00%  Thread6          [unknown]             [k] 0xffffffffffffffff
+    4.46%     0.09%  httpserver_test  httpserver_test       [.] muduo::EventLoop::queueInLoop(std::function<void ()> const&)
+    4.41%     0.00%  Thread15         [unknown]             [k] 0xffffffffffffffff
+    4.36%     0.00%  Thread1          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.36%     0.00%  Thread1          [kernel.kallsyms]     [k] do_syscall_64
+    4.35%     0.00%  Thread2          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.35%     0.00%  Thread2          [kernel.kallsyms]     [k] do_syscall_64
+    4.33%     0.00%  Thread1          [kernel.kallsyms]     [k] x64_sys_call
+    4.32%     0.05%  Thread4          [kernel.kallsyms]     [k] x64_sys_call
+    4.28%     0.00%  Thread11         [unknown]             [k] 0xffffffffffffffff
+    4.23%     0.00%  Thread6          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.21%     0.09%  httpserver_test  httpserver_test       [.] muduo::EPollPoller::poll(int, std::vector<muduo::Channel*, std::allocator<muduo
+    4.16%     0.00%  Thread8          [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    4.16%     0.19%  Thread8          [kernel.kallsyms]     [k] do_syscall_64
+    4.16%     0.00%  Thread8          [kernel.kallsyms]     [k] x64_sys_call
+    4.05%     0.00%  Thread6          [kernel.kallsyms]     [k] do_syscall_64
+    3.99%     0.00%  Thread3          [kernel.kallsyms]     [k] x64_sys_call
+    3.96%     0.00%  Thread11         [kernel.kallsyms]     [k] entry_SYSCALL_64_after_hwframe
+    3.96%     0.00%  Thread11         [kernel.kallsyms]     [k] do_syscall_64
```
