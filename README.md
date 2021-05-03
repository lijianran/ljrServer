# ljrServer

lijianran's Internet Server

## 环境

- vscode remote-container
- docker ubuntu20.04
- gcc g++ 9.3
- gdb 9.2
- cmake 3.16.3
- boost 1.71
- yaml-cpp 0.6.3

## 开发日志

- 2021.04.23

    复习计算机网络，开始学习C++网络服务器开发

- 2021.04.25

1. 初始化仓库
2. 初步完成日志log模块

- 2021.04.26

1. 开始配置config模块
2. 配置boost、yaml-cpp库
3. config模块支持简单类型的配置
4. STL容器偏特化，支持了vector、list、set、map、unordered_set、unordered_map
5. 支持自定义类型解析配置
6. 支持监听配置变更事件

- 2021.04.27

1. 日志系统、配置系统整合
2. 完善日志模块和配置模块
3. 开始设计线程模块
4. 设计互斥量、信号量
5. 使用Spinlock自旋锁替换Mutex，提高日志文件的写入性能
6. 写文件时周期reopen，防止文件删除
7. 配置模块增加读写锁

- 2021.04.28

1. 设计协程模块
2. 添加宏定义模块，增加断言
3. 添加Backtrace查看函数调用堆栈，方便调试
4. 设计协程调度模块，作为线程池来分配一组线程，并可以调度协程到指定线程上执行

- 2021.04.29

1. 设计IO协程调度模块
2. 添加定时器功能 ms

- 2021.04.30

1. 开始服务器HOOK模块
2. hook成功sleep，同步方式实现异步功能
3. 添加文件句柄类及句柄管理器，识别句柄是否是socket句柄
4. hook socket相关的系统函数

- 2021.05.01

1. 添加网络地址类，包括网络地址虚基类、IP地址类、IPv4类、IPv6类、Unix地址类、未知类
2. 添加字节序处理类，区分大小端机器，解决网络字节序和本地字节序的转换
3. 支持转换字符文本格式的地址

- 2021.05.02

1. 封装socket，包括connect、accept、read、write、close
2. 封装字节序列化类ByteArray，方便序列化和反序列化