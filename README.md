
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


## TODO:

* 性能调优