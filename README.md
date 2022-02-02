# ljrServer

lijianran's Internet Server

## 环境

-   ubuntu 20.04
-   gcc g++ 9.3
-   gdb 9.2
-   cmake 3.16.3
-   boost 1.71
-   yaml-cpp 0.6.3
-   ragel 6.10
-   http-parser

## 开发日志

-   2021.04.23

    复习计算机网络，开始学习 C++ 网络服务器开发

-   2021.04.25

1. 初始化仓库
2. 初步完成日志 log 模块

-   2021.04.26

1. 开始配置 config 模块
2. 配置 boost yaml-cpp 库
3. config 模块支持简单类型的配置
4. STL 容器偏特化，支持了 vector、list、set、map、unordered_set、unordered_map
5. 支持自定义类型解析配置
6. 支持监听配置变更事件

-   2021.04.27

1. 日志系统、配置系统整合
2. 完善日志模块和配置模块
3. 开始设计线程模块
4. 设计互斥量、信号量
5. 使用 Spinlock 自旋锁替换 Mutex，提高日志文件的写入性能
6. 写文件时周期 reopen，防止文件删除
7. 配置模块增加读写锁

-   2021.04.28

1. 设计协程模块
2. 添加宏定义模块，增加断言
3. 添加 Backtrace 查看函数调用堆栈，方便调试
4. 设计协程调度模块，作为线程池来分配一组线程，并可以调度协程到指定线程上执行

-   2021.04.29

1. 设计 IO 协程调度模块
2. 添加定时器功能 ms

-   2021.04.30

1. 开始服务器 HOOK 模块
2. hook 成功 sleep，同步方式实现异步功能
3. 添加文件句柄类及句柄管理器，识别句柄是否是 socket 句柄
4. hook socket 相关的系统函数

-   2021.05.01

1. 添加网络地址类，包括网络地址虚基类、IP 地址类、IPv4 类、IPv6 类、Unix 地址类、未知类
2. 添加字节序处理类，区分大小端机器，解决网络字节序和本地字节序的转换
3. 支持转换字符文本格式的地址

-   2021.05.02

1. 封装 socket，包括 connect、accept、read、write、close
2. 封装字节序列化类 ByteArray，方便序列化和反序列化

-   2021.05.03

1. 封装 HTTP 协议，针对 HTTP1.1 部分 API，主要实现 HttpRequest、HttpResponse
2. 使用 ragel 有限状态机，解析 HTTP 报文

-   2021.05.04

1. 封装 TcpServer，基于 TcpServer 实现一个 EchoServer 服务器
2. 针对文件/Socket，封装 Stream
3. 封装 HttpSession
4. 基于 TcpServer 封装 HttpServer，结合 HttpSession 接收 Client 的请求数据
5. 封装 HttpServlet

-   2021.05.05

1. 封装 HttpConnection
2. 解析 chunked 数据
3. 解析 uri
4. 封装 http 连接池，支持长连接
