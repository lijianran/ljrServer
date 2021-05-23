
// #include <iostream>
#include "../ljrServer/log.h"
// #include "../ljrServer/util.h"

int main(int argc, char const *argv[])
{

    ljrserver::Logger::ptr logger(new ljrserver::Logger);

    logger->addAppender(ljrserver::LogAppender::ptr(new ljrserver::StdoutLogAppender));

    // ljrserver::LogEvent::ptr event(new ljrserver::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));
    // ljrserver::LogEvent::ptr event(new ljrserver::LogEvent(__FILE__, __LINE__, 0, ljrserver::GetThreadId(), ljrserver::GetFiberId(), time(0)));
    // event->getSS() << "hello lijianran server log";
    // logger->log(ljrserver::LogLevel::DEBUG, event);

    ljrserver::FileLogAppender::ptr file_appender(new ljrserver::FileLogAppender("./log.txt"));
    ljrserver::LogFormatter::ptr file_format(new ljrserver::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(file_format);
    // file_appender->setLevel(ljrserver::LogLevel::ERROR);

    logger->addAppender(file_appender);

    LJRSERVER_LOG_INFO(logger) << "test info logger";
    LJRSERVER_LOG_ERROR(logger) << "test error logger";

    LJRSERVER_LOG_FORMAT_ERROR(logger, "test format error logger %s", "test-string");
    LJRSERVER_LOG_FORMAT_ERROR(logger, "test format error logger %d", 110);

    auto l = ljrserver::LoggerMgr::GetInstance()->getLogger("xx");
    LJRSERVER_LOG_DEBUG(l) << "test loggermanager";

    return 0;
}
