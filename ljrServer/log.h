
#ifndef __LJRSERVER_LOG_H__
#define __LJRSERVER_LOG_H__

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>
// ... / __VA_ARGS__
#include <stdarg.h>

#include "util.h"
#include "singleton.h"
#include "thread.h"

// 流式输出
#define LJRSERVER_LOG_LEVEL(logger, level)                                    \
    if (logger->getLevel() <= level)                                          \
    ljrserver::LogEventWrap(                                                  \
        ljrserver::LogEvent::ptr(new ljrserver::LogEvent(                     \
            logger, level, __FILE__, __LINE__, 0, ljrserver::GetThreadId(),   \
            ljrserver::GetFiberId(), time(0), ljrserver::Thread::GetName()))) \
        .getSS()

#define LJRSERVER_LOG_DEBUG(logger) \
    LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::DEBUG)
#define LJRSERVER_LOG_INFO(logger) \
    LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::INFO)
#define LJRSERVER_LOG_WARN(logger) \
    LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::WARN)
#define LJRSERVER_LOG_ERROR(logger) \
    LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::ERROR)
#define LJRSERVER_LOG_FATAL(logger) \
    LJRSERVER_LOG_LEVEL(logger, ljrserver::LogLevel::FATAL)

// 格式化输出
#define LJRSERVER_LOG_FORMAT_LEVEL(logger, level, fmt, ...)                   \
    if (logger->getLevel() <= level)                                          \
    ljrserver::LogEventWrap(                                                  \
        ljrserver::LogEvent::ptr(new ljrserver::LogEvent(                     \
            logger, level, __FILE__, __LINE__, 0, ljrserver::GetThreadId(),   \
            ljrserver::GetFiberId(), time(0), ljrserver::Thread::GetName()))) \
        .getEvent()                                                           \
        ->format(fmt, __VA_ARGS__)

#define LJRSERVER_LOG_FORMAT_DEBUG(logger, fmt, ...)                    \
    LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::DEBUG, fmt, \
                               __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_INFO(logger, fmt, ...)                    \
    LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::INFO, fmt, \
                               __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_WARN(logger, fmt, ...)                    \
    LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::WARN, fmt, \
                               __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_ERROR(logger, fmt, ...)                    \
    LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::ERROR, fmt, \
                               __VA_ARGS__)
#define LJRSERVER_LOG_FORMAT_FATAL(logger, fmt, ...)                    \
    LJRSERVER_LOG_FORMAT_LEVEL(logger, ljrserver::LogLevel::FATAL, fmt, \
                               __VA_ARGS__)

// root 日志
#define LJRSERVER_LOG_ROOT() ljrserver::LoggerMgr::GetInstance()->getRoot()

// 按 name 找到 logger 没有则创建
#define LJRSERVER_LOG_NAME(name) \
    ljrserver::LoggerMgr::GetInstance()->getLogger(name)

namespace ljrserver {
class Logger;
class LoggerManager;

/**
 * @brief Class 级别 (日志器 logger / 日志输出地 appender / 日志格式 formatter)
 * \n
 *
 * enum Level { UNKNOW = 0, DEBUG, INFO, WARN, ERROR, FATAL };
 */
class LogLevel {
public:
    enum Level { UNKNOW = 0, DEBUG, INFO, WARN, ERROR, FATAL };

    /**
     * @brief 日志级别 -> 字符串
     *
     * @param level
     * @return const char*
     */
    static const char *ToString(LogLevel::Level level);

    /**
     * @brief 字符串 -> 日志级别
     *
     * @param str
     * @return LogLevel::Level
     */
    static LogLevel::Level FromString(const std::string &str);
};

/**
 * @brief Class 日志事件
 *
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    /**
     * @brief LogEvent 构造函数
     *
     * @param logger 日志器
     * @param level 日志级别
     * @param file 文件名
     * @param line 行号
     * @param elapse 毫秒数
     * @param thread_id 线程 id
     * @param fiber_id 协程 id
     * @param time 时间戳
     * @param thread_name 线程名称
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char *file, int32_t line, uint32_t elapse,
             uint32_t thread_id, uint32_t fiber_id, uint64_t time,
             const std::string &thread_name);

    const char *getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberID; }
    uint64_t getTime() const { return m_time; }
    const std::string &getThreadName() const { return m_threadName; }
    std::string getContent() const { return m_ss.str(); }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }

    // 获得事件内容的string流
    std::stringstream &getSS() { return m_ss; }

    /**
     * @brief format 格式化日志内容
     *
     * @param fmt
     * @param ...
     */
    void format(const char *fmt, ...);

    /**
     * @brief format 格式化日志内容
     *
     * @param fmt
     * @param al
     */
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
    // 线程名称
    std::string m_threadName;

    // 事件内容 (支持流式输入和 format 格式化输入)
    std::stringstream m_ss;

    // 日志器
    std::shared_ptr<Logger> m_logger;
    // 日志级别
    LogLevel::Level m_level;
};

/**
 * @brief Class 日志事件 LogEvent 的包装器
 *
 * 析构函数输出日志 LogEvent
 */
class LogEventWrap {
public:
    /**
     * @brief Construct a new Log Event Wrap object 包装日志事件
     *
     * @param event
     */
    LogEventWrap(LogEvent::ptr event);

    /**
     * @brief Destroy the Log Event Wrap object
     *
     * 输出日志
     */
    ~LogEventWrap();

    /**
     * @brief 获取日志内容流
     *
     * @return std::stringstream&
     */
    std::stringstream &getSS();

    /**
     * @brief 获取日志事件指针
     *
     * @return LogEvent::ptr
     */
    LogEvent::ptr getEvent() const;

private:
    // 日志事件指针
    LogEvent::ptr m_event;
};

/**
 * @brief Class 日志格式器
 *
 */
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    /**
     * @brief 日志格式器构造函数
     *
     * 调用 init 解析格式字符串
     *
     * @param pattern 格式字符串
     */
    LogFormatter(const std::string &pattern);

    /**
     * @brief 格式化日志内容
     *
     * @param logger
     * @param level
     * @param event
     * @return std::string
     */
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr event);

    /**
     * @brief 格式化日志内容
     *
     * @param ofs
     * @param logger
     * @param level
     * @param event
     * @return std::ostream&
     */
    std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger,
                         LogLevel::Level level, LogEvent::ptr event);

    /**
     * @brief 格式字符串解析是否有误
     *
     * @return true 有错误
     * @return false 无错误
     */
    bool isError() const { return m_error; }

    /**
     * @brief 获取格式字符串
     *
     * @return std::string 格式字符串
     */
    std::string getPattern() const { return m_pattern; }

public:
    /**
     * @brief 格式项父类 抽象类不能实例化
     *
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        /**
         * @brief 基类析构函数设置为虚函数
         *
         */
        virtual ~FormatItem() {}

        /**
         * @brief 纯虚函数 格式化日志内容，输出内容到 ostream
         *
         * 子类需要重载 format 函数，否则也是抽象类，无法实例化
         * @param os
         * @param logger
         * @param level
         * @param event
         */
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;
    };

private:
    /**
     * @brief 日志格式解析
     *
     */
    void init();

private:
    // 格式字符串
    std::string m_pattern;

    // 格式子类的 vector 集合，由 init 解析设置
    std::vector<FormatItem::ptr> m_items;

    // 格式解析是否有错误
    bool m_error = false;
};

/**
 * @brief Class 日志输出地
 *
 * 纯虚基类 抽象类 -> StdoutLogAppender / FileLogAppender
 *
 * 抽象类无法实例化
 */
class LogAppender {
    friend class Logger;

public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief 基类的析构函数需要设置为虚函数
     *
     */
    virtual ~LogAppender() {}

    /**
     * @brief 纯虚函数 子类需要重载 日志打印
     *
     * @param logger
     * @param level
     * @param event
     */
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    /**
     * @brief 纯虚函数 子类需要重载 转 yaml 字符串
     *
     * @return std::string
     */
    virtual std::string toYamlString() = 0;

    /**
     * @brief 获取 appender 的日志格式
     *
     * @return LogFormatter::ptr
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief 设置 appender 的日志格式
     *
     * @param formatter
     */
    void setFormatter(LogFormatter::ptr formatter);

    /**
     * @brief 获取 appender 的日志等级
     *
     * @return LogLevel::Level
     */
    LogLevel::Level getLevel() const { return m_level; }

    /**
     * @brief 设置 appender 的日志等级
     *
     * @param level
     */
    void setLevel(LogLevel::Level level) { m_level = level; }

protected:
    // 忘记初始化 level
    LogLevel::Level m_level = LogLevel::DEBUG;

    // appender 是否有设定的格式
    bool m_hasFormatter = false;

    // appender 的锁
    MutexType m_mutex;

    // appender 的格式
    LogFormatter::ptr m_formatter;
};

/**
 * @brief Class 日志器
 *
 * enable_shared_from_this 可以获取自身引用
 *
 */
class Logger : public std::enable_shared_from_this<Logger> {
    // LoggerManager 友元可以设置 m_root 等成员变量
    friend class LoggerManager;

public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    /**
     * @brief 日志器 logger 构造函数
     *
     * 默认名称: root
     * 默认级别: debug
     *
     * @param name 日志器名称
     */
    Logger(const std::string &name = "root");

    /**
     * @brief 日志器 logger 打印日志
     *
     * @param level 要打印的日志的级别
     * @param event 要打印的日志事件
     */
    void log(LogLevel::Level level, LogEvent::ptr event);
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    /**
     * @brief 添加 logger 的appender
     *
     * @param appender
     */
    void addAppender(LogAppender::ptr appender);

    /**
     * @brief 删除 logger 的appender
     *
     * @param appender
     */
    void delAppender(LogAppender::ptr appender);

    /**
     * @brief 清空 logger 的 appender
     *
     */
    void clearAppenders();

    /**
     * @brief Get the Level object
     *
     * 获取日志器 logger 的等级
     *
     * @return LogLevel::Level
     */
    LogLevel::Level getLevel() const { return m_level; }

    /**
     * @brief Set the Level object
     *
     * 设置日志器 logger 的等级
     *
     * @param level
     */
    void setLevel(LogLevel::Level level) { m_level = level; }

    /**
     * @brief 获取日志器 logger 的名称
     *
     * @return const std::string& 常量字符串
     */
    const std::string &getName() const { return m_name; }

    /**
     * @brief 获取日志器 logger 的日志格式
     *
     * @return LogFormatter::ptr 日志格式类指针
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief 设置日志器 logger 的日志格式
     *
     * @param val LogFormatter::ptr 日志格式类指针
     */
    void setFormatter(LogFormatter::ptr val);

    /**
     * @brief 设置日志器 logger 的日志格式（参数类型不同 重载）
     *
     * @param val const std::string 格式字符串
     */
    void setFormatter(const std::string &val);

    /**
     * @brief 转 yaml 字符串
     *
     * @return std::string
     */
    std::string toYamlString();

private:
    // 日志器 logger 的名称
    std::string m_name;

    // 日志器 logger 的级别
    LogLevel::Level m_level;

    // 日志器 logger 的 Appender List
    std::list<LogAppender::ptr> m_appenders;

    // 日志器 logger 的日志格式器
    LogFormatter::ptr m_formatter;

    // 默认日志器 root (由 LoggerManager 设置)
    Logger::ptr m_root;

    // 日志器 logger 的锁
    MutexType m_mutex;
};

/**
 * @brief Class 输出到控制台的 Appender
 *
 * LogAppender 的子类
 */
class StdoutLogAppender : public LogAppender {
    friend class Logger;

public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    /**
     * @brief 纯虚函数的重载实现 输出到控制台的日志
     *
     * @param logger 日志器
     * @param level 要打印的日志的级别
     * @param event 要打印的日志事件
     */
    void log(Logger::ptr logger, LogLevel::Level level,
             LogEvent::ptr event) override;

    /**
     * @brief 纯虚函数的重载实现 转 yaml 字符串
     *
     * @return std::string
     */
    std::string toYamlString() override;

private:
};

/**
 * @brief Class 输出到文件中的 Appender
 *
 * LogAppender 的子类
 */
class FileLogAppender : public LogAppender {
    friend class Logger;

public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    /**
     * @brief 文件日志构造函数
     *
     * @param filename 日志文件名
     */
    FileLogAppender(const std::string &filename);

    /**
     * @brief 纯虚函数的实现 输出到文件的日志
     *
     * @param logger 日志器
     * @param level 要打印的日志的级别
     * @param event 要打印的日志事件
     */
    void log(Logger::ptr logger, LogLevel::Level level,
             LogEvent::ptr event) override;

    /**
     * @brief 纯虚函数的实现 转 yaml 字符串
     *
     * @return std::string
     */
    std::string toYamlString() override;

    /**
     * @brief 打开日志文件
     *
     * @return true
     * @return false
     */
    bool reopen();

private:
    // 日志文件名
    std::string m_filename;

    // 日志文件流
    std::ofstream m_filestream;

    // 上次打开时间
    uint64_t m_lastTime = 0;
};

/**
 * @brief Class 日志管理器
 *
 */
class LoggerManager {
public:
    typedef Spinlock MutexType;

    /**
     * @brief Construct a new Logger Manager:: Logger Manager object
     *
     * 日志管理器构造函数
     * 构造默认 m_root
     */
    LoggerManager();

    /**
     * @brief 找 logger 日志器
     *
     * 没有找到 logger 则创建
     *
     * @param name 日志器名称
     * @return Logger::ptr 日志器指针
     */
    Logger::ptr getLogger(const std::string &name);

    // void init();

    /**
     * @brief Get the Root object 获取默认日志器 m_root
     *
     * @return Logger::ptr
     */
    Logger::ptr getRoot() const { return m_root; }

    /**
     * @brief 转 yaml 字符串
     *
     * @return std::string
     */
    std::string toYamlString();

private:
    // 日志器列表 map
    std::map<std::string, Logger::ptr> m_loggers;

    // 默认的日志器
    Logger::ptr m_root;

    // 日志管理器的自旋锁
    MutexType m_mutex;
};

/**
 * @brief 单例模式 日志管理器
 *
 */
typedef ljrserver::Singleton<LoggerManager> LoggerMgr;

}  // namespace ljrserver

#endif  // __LJRSERVER_LOG_H__