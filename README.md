
* 支持踢掉空闲连接
* 支持web视频播放、图片展示，使用内存与文件大小无关，只与连接数有关。

## 测试：

[webbench-1.5](https://github.com/qinguoyi/TinyWebServer/tree/master/test_pressure/webbench-1.5)。

同一台电脑.


* 内存：8G
* CPU: [AMD Ryzen™ 9 7945HX](https://www.amd.com/en/products/processors/laptop/ryzen/7000-series/amd-ryzen-9-7945hx.html)，16核，32线程

### wsl环境

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


### ubuntu22.04 物理机环境

```
$ ./webbench -c 10500 -t 5 --get -1  http://127.0.0.1:8008/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8008/
10500 clients, running 5 sec.

Speed=4506936 pages/min, 48448356 bytes/sec.
Requests: 375578 susceed, 0 failed.

```

* youshuang

```
$ ./webbench -c 10500 -t 5 --get -1  http://127.0.0.1:9006/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:9006/
10500 clients, running 5 sec.

Speed=1241052 pages/min, 2316630 bytes/sec.
Requests: 103421 susceed, 0 failed.
```

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

## TODO:

* 性能调优