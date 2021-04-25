
#include <iostream>
#include "../ljrServer/log.h"

int main(int argc, char const *argv[])
{

    ljrserver::Logger::ptr logger(new ljrserver::Logger);

    logger->addAppender(ljrserver::LogAppender::ptr(new ljrserver::StdoutLogAppender));

    ljrserver::LogEvent::ptr event(new ljrserver::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));
    event->getSS() << "hello lijianran server log";

    logger->log(ljrserver::LogLevel::DEBUG, event);
    return 0;
}
