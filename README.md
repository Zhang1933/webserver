
* 支持踢掉空闲连接
* 支持web视频播放、图片展示，使用内存与文件大小无关，只与连接数有关。

## 测试：

[webbench-1.5](https://github.com/qinguoyi/TinyWebServer/tree/master/test_pressure/webbench-1.5)。

同一台电脑，wsl环境。

* 内存：8G
* CPU: [AMD Ryzen™ 9 7945HX](https://www.amd.com/en/products/processors/laptop/ryzen/7000-series/amd-ryzen-9-7945hx.html)，16核，32线程


两个服务器程序均开启8线程，返回相同页面测试，设置同样大小的文件符软限制。

```bash
$ ./webbench -c 10500 -t 5 --get -1  http://172.18.82.9:8008/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://172.18.82.9:8008/
10500 clients, running 5 sec.

Speed=326604 pages/min, 4018540 bytes/sec.
Requests: 27217 susceed, 0 failed.
```

支持上万连接。

与[youshuang](https://github.com/qinguoyi/TinyWebServer)的webserver比较：
```bash
$ ./webbench -c 10500 -t 5 --get -1  http://172.18.82.9:9006/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://172.18.82.9:9006/
10500 clients, running 5 sec.

Speed=132492 pages/min, 247296 bytes/sec.
Requests: 11041 susceed, 0 failed.
```


* 16线程池大小压力测试：

```bash
$ ./webbench -c 10500 -t 5 --get -1  http://172.18.82.9:8008/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://172.18.82.9:8008/
10500 clients, running 5 sec.

Speed=346536 pages/min, 4148638 bytes/sec.
Requests: 28878 susceed, 0 failed.
```

# HttpServer


# Redis API

Lazy conneciton：
1. 线程向连接池申请连接时，加锁,如果发现已申请连接没达到设定的连接池大小，申请新的空连接，尝试用密码连接redis
    如果在向连接池申请连接时，发现已申请连接达到设定的连接池大小，连接池中没有连接了，等待其他线程释放连接或坏连接变好。
    

## 后台自动重连逻辑


1. 在连接池没满时，申请新连接的线程会尝试用密码连接redis，失败时，向重连接事件循环中注册重连接任务，向上抛出异常。
2. 如果连接成功后，发送命令后得到redis回复失败，连接可能断开，向重连接事件循环中注册重连接任务，向上抛出异常，表示命令没有执行成功。

* 后台重连接事件循环线程

1. 运行重连接事件循环的线程负责重连，重连成功后，放回连接池。若任然失败，隔(2^1,2^0,,,2^10)秒后自动重试连接。
2. 自动重连接的实现方式为向事件循环中反复注册定时重连接任务，根据失败次数改变定时时间。

用户可用broken_connection_cnt与连接池大小判断可用连接数。

## TODO:


* 性能调优