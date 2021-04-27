
#ifndef __LJRSERVER_LOG_H__
#define __LJRSERVER_LOG_H__

#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"

// 流式输出
#define LJRSERVER_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level)       \
    ljrserver::LogEventWrap(ljrserver::LogEvent::ptr(new ljrserver::LogEvent(logger, level, __FILE__, __LINE__, 0, ljrserver::GetThreadId(), ljrserver::GetFiberId(), time(0)))).getSS()

#define LJRSERVER_LOG_DEBUG(logger) LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::DEBUG)
#define LJRSERVER_LOG_INFO(logger) LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::INFO)
#define LJRSERVER_LOG_WARN(logger) LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::WARN)
#define LJRSERVER_LOG_ERROR(logger) LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::ERROR)
#define LJRSERVER_LOG_FATAL(logger) LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::FATAL)

// 格式化输出
#define LJRSERVER_LOG_FORMAT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level)                        \
    ljrserver::LogEventWrap(ljrserver::LogEvent::ptr(new ljrserver::LogEvent(logger, level, __FILE__, __LINE__, 0, ljrserver::GetThreadId(), ljrserver::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define LJRSERVER_LOG_FORMAT_DEBUG(logger, fmt, ...) LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_INFO(logger, fmt, ...) LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::INFO, fmt, __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_WARN(logger, fmt, ...) LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::WARN, fmt, __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_ERROR(logger, fmt, ...) LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_FATAL(logger, fmt, ...) LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::FATAL, fmt, __VA_ARGS__)

// root日志
#define LJRSERVER_LOG_ROOT() ljrserver::LoggerMgr::GetInstance()->getRoot()

// 找到log
#define LJRSERVER_LOG_NAME(name) ljrserver::LoggerMgr::GetInstance()->getLogger(name)

namespace ljrserver
{
    class Logger;
    class LoggerManager;

    // 日志级别
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL
        };

        static const char *ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string &str);
    };

    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                 const char *file, int32_t line, uint32_t elapse,
                 uint32_t thread_id, uint32_t fiber_id, uint64_t time);

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberID; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        std::stringstream &getSS() { return m_ss; }

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        // 文件名
        const char *m_file = nullptr;
        // 行号
        int32_t m_line = 0;
        // 程序启动开始到现在的毫秒数
        uint32_t m_elapse = 0;
        // 线程id
        uint32_t m_threadId = 0;
        // 协程id
        uint32_t m_fiberID = 0;
        // 时间戳
        uint64_t m_time = 0;
        // 事件内容
        std::stringstream m_ss;
        // 日志器
        std::shared_ptr<Logger> m_logger;
        // 日志级别
        LogLevel::Level m_level;
    };

    // 日志事件包装器
    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr event);
        ~LogEventWrap();

        std::stringstream &getSS();

        LogEvent::ptr getEvent() const;

    private:
        LogEvent::ptr m_event;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        bool isError() const { return m_error; }

        std::string getPattern() const { return m_pattern; }

    public:
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;

            virtual ~FormatItem() {}

            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

    private:
        void init();

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    // 日志输出地
    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;

        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        virtual std::string toYamlString() = 0;

        LogFormatter::ptr getFormatter() const { return m_formatter; }
        void setFormatter(LogFormatter::ptr formatter);

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }

    protected:
        // 忘记初始化level
        LogLevel::Level m_level = LogLevel::DEBUG;

        bool m_hasFormatter = false;

        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string &name = "root");

        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }

        const std::string &getName() const { return m_name; }

        LogFormatter::ptr getFormatter();
        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);

        std::string toYamlString();

    private:
        // 日志名称
        std::string m_name;
        // 日志级别
        LogLevel::Level m_level;
        // Appender集合
        std::list<LogAppender::ptr> m_appenders;
        // 日志格式器
        LogFormatter::ptr m_formatter;

        Logger::ptr m_root;
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;

    private:
    };

    // 输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string &filename);

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;

        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    // 日志管理器
    class LoggerManager
    {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);

        void init();

        Logger::ptr getRoot() const { return m_root; }

        std::string toYamlString();

    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    typedef ljrserver::Singleton<LoggerManager> LoggerMgr;

}

#endif