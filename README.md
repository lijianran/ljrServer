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