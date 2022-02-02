
// #include <iostream>
#include "../ljrServer/log.h"
// #include "../ljrServer/util.h"

int main(int argc, char const *argv[]) {
    // 添加日志器
    ljrserver::Logger::ptr logger(new ljrserver::Logger);

    // 添加 appender
    logger->addAppender(
        ljrserver::LogAppender::ptr(new ljrserver::StdoutLogAppender));

    // 日志事件
    // ljrserver::LogEvent::ptr event(new ljrserver::LogEvent(__FILE__,
    // __LINE__, 0, 1, 2, time(0)));

    // 日志事件 用宏函数流式输出或格式化输出
    // ljrserver::LogEvent::ptr event(new ljrserver::LogEvent(__FILE__,
    // __LINE__, 0, ljrserver::GetThreadId(), ljrserver::GetFiberId(),
    // time(0))); event->getSS() << "hello lijianran server log";

    // 打印日志 包装器在析构时打印日志
    // logger->log(ljrserver::LogLevel::DEBUG, event);

    // 文件日志
    ljrserver::FileLogAppender::ptr file_appender(
        new ljrserver::FileLogAppender("./log.txt"));

    // 日志格式
    ljrserver::LogFormatter::ptr file_format(
        new ljrserver::LogFormatter("%d%T%p%T%m%n"));
    
    // 设置格式
    file_appender->setFormatter(file_format);

    // 设置 appender 等级
    file_appender->setLevel(ljrserver::LogLevel::ERROR);

    // 为日志器添加
    logger->addAppender(file_appender);

    LJRSERVER_LOG_INFO(logger) << "test info logger";
    LJRSERVER_LOG_ERROR(logger) << "test error logger";

    LJRSERVER_LOG_FORMAT_ERROR(logger, "test format error logger %s",
                               "test-string");
    LJRSERVER_LOG_FORMAT_ERROR(logger, "test format error logger %d", 110);

    // 测试 LoggerMgr
    auto l = ljrserver::LoggerMgr::GetInstance()->getLogger("xx");
    LJRSERVER_LOG_DEBUG(l) << "test loggermanager";

    return 0;
}
