
#ifndef _LJRSERVER_LOG_H_
#define _LJRSERVER_LOG_H_

#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>

namespace ljrserver
{
    class Logger;

    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id,
                 uint32_t fiber_id, uint64_t time);

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberID; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::stringstream &getSS() { return m_ss; }

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
    };

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

        static const char *toString(LogLevel::Level level);
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

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
    };

    // 日志输出地
    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;

        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }

    protected:
        // 忘记初始化level
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
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

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }

        const std::string &getName() const { return m_name; }

    private:
        // 日志名称
        std::string m_name;
        // 日志级别
        LogLevel::Level m_level;
        // Appender集合
        std::list<LogAppender::ptr> m_appenders;
        // 日志格式器
        LogFormatter::ptr m_formatter;
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    private:
    };

    // 输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string &filename);

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };
}

#endif