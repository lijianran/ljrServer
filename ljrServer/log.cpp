
#include "log.h"
#include "config.h"

#include <iostream>
#include <functional>
#include <time.h>

namespace ljrserver {

/**
 * @brief FormatItem 十二种子类
 * %m -- 消息体
 * %p -- 级别 level
 * %r -- 启动后时间
 * %c -- 日志器 logger 名称
 * %t -- 线程 id
 * %F -- 协程 id
 * %N -- 线程名称
 * %d -- 时间
 * %f -- 文件名
 * %l -- 行号
 * %n -- 回车换行
 * %T -- tab 制表符
 */

/**
 * @brief 消息体 %m
 */
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getContent();
    }
};

/**
 * @brief 级别 %p
 */
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

/**
 * @brief 启动后时间 %r
 */
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

/**
 * @brief 日志器名称 %c
 */
class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        // os << logger->getName();
        os << event->getLogger()->getName();
    }
};

/**
 * @brief 线程 id %t
 */
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

/**
 * @brief 协程 id
 */
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

/**
 * @brief 线程名称 %N
 */
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

/**
 * @brief 时间 %d
 */
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string &format = "%Y.%m.%d %H:%M:%S")
        : m_format(format) {
        // 为什么？
        if (m_format.empty()) {
            m_format = "%Y.%m.%d %H:%M:%S";
        }
    }

    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buffer[64];
        strftime(buffer, sizeof(buffer), m_format.c_str(), &tm);
        os << buffer;
    }

private:
    std::string m_format;
};

/**
 * @brief 文件名 %f
 */
class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFile();
    }
};

/**
 * @brief 行号 %l
 */
class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getLine();
    }
};

/**
 * @brief 换行回车 %n
 */
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << std::endl;
    }
};

/**
 * @brief tab 制表符 %T
 */
class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << "\t";
    }
};

/**
 * @brief 字符串
 */
class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string &str = "") : m_string(str) {}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << m_string;
    }

private:
    std::string m_string;
};

/***************
LogEvent
***************/

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
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                   const std::string &thread_name)
    : m_file(file),
      m_line(line),
      m_elapse(elapse),
      m_threadId(thread_id),
      m_fiberID(fiber_id),
      m_time(time),
      m_threadName(thread_name),
      m_logger(logger),
      m_level(level) {
    // 初始化列表与成员变量定义顺序一致
    // 日志事件 event 需要内置日志器 logger
}

/**
 * @brief format 格式化日志内容
 *
 * @param fmt
 * @param ...
 */
void LogEvent::format(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

/**
 * @brief format 格式化日志内容
 *
 * @param fmt
 * @param al
 */
void LogEvent::format(const char *fmt, va_list al) {
    // 缓存
    char *buffer = nullptr;
    // 格式化
    int len = vasprintf(&buffer, fmt, al);
    if (len != -1) {
        // 输入到日志内容
        m_ss << std::string(buffer, len);
        // 释放缓存
        free(buffer);
    }
}

/***************
LogEventWrap
***************/

/**
 * @brief Construct a new Log Event Wrap object 包装日志事件
 *
 * @param event
 */
LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event) {}

/**
 * @brief Destroy the Log Event Wrap:: Log Event Wrap object
 *
 * 析构时 输出日志
 */
LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

/**
 * @brief 获取日志内容流
 *
 * @return std::stringstream&
 */
std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }

/**
 * @brief 获取日志事件指针
 *
 * @return LogEvent::ptr
 */
LogEvent::ptr LogEventWrap::getEvent() const { return m_event; }

/***************
LogAppender
***************/

/**
 * @brief 设置 appender 的日志格式
 *
 * @param formatter 日志格式的智能指针
 */
void LogAppender::setFormatter(LogFormatter::ptr formatter) {
    MutexType::Lock lock(m_mutex);

    m_formatter = formatter;

    if (m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

/**
 * @brief 获取 appender 的日志格式
 *
 * @return LogFormatter::ptr 日志格式的智能指针类型
 */
LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);

    return m_formatter;
}

/***************
LogLevel
***************/

/**
 * @brief 日志级别 -> 字符串
 *
 * @param level
 * @return const char*
 */
const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        // break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);

#undef XX
        default:
            return "UNKNOW";
            // break;
    }
    // return "UNKNOW";
}

/**
 * @brief 字符串 -> 日志级别
 *
 * @param str
 * @return LogLevel::Level
 */
LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v)            \
    if (str == #v) {            \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;
#undef XX
}

/***************
Logger
***************/

/**
 * @brief 日志器 logger 构造函数
 *
 * 默认名称: root
 * 默认级别: debug
 *
 * @param name 日志器名称
 */
Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::DEBUG) {
    // 日志器 logger 默认格式
    m_formatter.reset(new LogFormatter(
        "%d{%Y.%m.%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // 日期时间 线程id 线程名称 协程id 日志级别 日志器名称 文件:行号 日志消息 \n
}

/**
 * @brief 转 yaml 字符串
 *
 * @return std::string
 */
std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["name"] = m_name;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }

    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for (auto &i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

/**
 * @brief 获取日志器 logger 的日志格式
 *
 * @return LogFormatter::ptr 日志格式类指针
 */
LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);

    return m_formatter;
}

/**
 * @brief 设置日志器 logger 的日志格式
 *
 * @param val LogFormatter::ptr 日志格式类指针
 */
void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);

    m_formatter = val;

    // 将该日志器 logger 的所有没有格式的 appender 都设置一遍
    for (auto &appender : m_appenders) {
        // logger 是 appender 的友元类，可以直接操作 appender 的锁
        MutexType::Lock ll(appender->m_mutex);

        // logger 是 appender 的友元类，可以直接访问 appender 的 m_hasFormatter
        if (!appender->m_hasFormatter) {
            // 如果 appender 没有设定的格式，则把 logger 的格式给 appender
            // logger 是 appender 的友元类，可以直接操作 appender 的 m_formatter
            appender->m_formatter = m_formatter;
        }
    }
}

/**
 * @brief 设置日志器 logger 的日志格式（参数类型不同 重载）
 *
 * @param val const std::string 格式字符串
 */
void Logger::setFormatter(const std::string &val) {
    // LogFormatter 构造函数调用 init 解析格式字符串
    ljrserver::LogFormatter::ptr new_formatter(
        new ljrserver::LogFormatter(val));

    // 格式解析是否有错
    if (new_formatter->isError()) {
        std::cout << "Logger setFormatter error name = " << m_name
                  << " value = " << val << " invalid formatter" << std::endl;
        return;
    }

    // m_formatter = new_val;
    setFormatter(new_formatter);
}

/**
 * @brief 添加 logger 的appender
 *
 */
void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);

    // 少了!
    if (!appender->getFormatter()) {
        MutexType::Lock ll(appender->m_mutex);

        // 如果 appender 没有定义格式，将 logger 的默认格式传给 appender
        appender->m_formatter = m_formatter;
        // logger 是 appender 的友元类，可以直接访问 appender 的 m_hasFormatter
        // appender->setFormatter(m_formatter);
    }

    m_appenders.push_back(appender);
}

/**
 * @brief 删除 logger 的appender
 *
 */
void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);

    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

/**
 * @brief 清空 logger 的appender
 *
 */
void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);

    m_appenders.clear();
}

/**
 * @brief 日志器 logger 打印日志
 *
 * @param level 要打印的日志的级别
 * @param event 要打印的日志事件
 */
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    // 日志器的级别 <= 要打印的日志的级别
    if (m_level <= level) {
        // enable_shared_from_this 获取自身指针
        auto self = shared_from_this();

        // 上锁
        MutexType::Lock lock(m_mutex);

        if (!m_appenders.empty()) {
            // appender list 不为空
            for (auto &appender : m_appenders) {
                // 调用 logger 的 appender 打印日志
                // 虚函数 动态多态
                // 调用 StdoutLogAppender / FileLogAppender 的 log 函数实现
                appender->log(self, level, event);
            }
        } else if (m_root) {
            // 没有设置 appender 使用默认 root 日志器
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

/***************
LogAppender - stdout
***************/

/**
 * @brief 纯虚函数的重载实现 输出到控制台的日志
 *
 * @param logger 日志器
 * @param level 要打印的日志的级别
 * @param event 要打印的日志事件
 */
void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                            LogEvent::ptr event) {
    // appender 的级别 <= 要打印的日志的级别
    if (m_level <= level) {
        MutexType::Lock lock(m_mutex);

        // std::cout << m_formatter->format(logger, level, event);
        m_formatter->format(std::cout, logger, level, event);
    }
}

/**
 * @brief 纯虚函数的重载实现 转 yaml 字符串
 *
 * @return std::string
 */
std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }

    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

/***************
LogAppender - file
***************/

/**
 * @brief 文件日志构造函数
 *
 * @param filename 日志文件名
 */
FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
    reopen();
}

/**
 * @brief 纯虚函数的实现 输出到文件的日志
 *
 * @param logger 日志器
 * @param level 要打印的日志的级别
 * @param event 要打印的日志事件
 */
void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                          LogEvent::ptr event) {
    // appender 的级别 <= 要打印的日志的级别
    if (m_level <= level) {
        uint64_t now = time(0);
        if (now != m_lastTime) {
            reopen();
            m_lastTime = now;
        }

        MutexType::Lock lock(m_mutex);

        m_filestream << m_formatter->format(logger, level, event);
    }
}

/**
 * @brief 纯虚函数的实现 转 yaml 字符串
 *
 * @return std::string
 */
std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }

    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

/**
 * @brief 打开日志文件
 *
 * @return true
 * @return false
 */
bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);

    if (m_filestream) {
        m_filestream.close();
    }

    m_filestream.open(m_filename, std::ofstream::app);
    return !!m_filestream;
}

/***************
LogFormatter
***************/

/**
 * @brief 日志格式器构造函数
 *
 * 调用 init 解析格式字符串
 *
 * @param pattern 格式字符串
 */
LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {
    // 解析格式字符串
    init();
}

/**
 * @brief 格式化日志内容
 *
 * @param logger
 * @param level
 * @param event
 * @return std::string
 */
std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
    // 缓存字符串流
    std::stringstream ss;
    for (auto &item : m_items) {
        // 调用特定的格式类，格式化内容到字符串缓存流 ss
        item->format(ss, logger, level, event);
    }
    // 返回字符串
    return ss.str();
}

/**
 * @brief 格式化日志内容
 *
 * @param ofs
 * @param logger
 * @param level
 * @param event
 * @return std::ostream&
 */
std::ostream &LogFormatter::format(std::ostream &ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level level, LogEvent::ptr event) {
    for (auto &item : m_items) {
        // 调用特定的格式类，格式化内容到字符串流 ofs
        item->format(ofs, logger, level, event);
    }
    // 返回字符串流
    return ofs;
}

/**
 * @brief 日志格式解析
 *
 */
void LogFormatter::init() {
    // std::cout << "formatter init" << std::endl;
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if ((i + 1) < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int format_status = 0;
        size_t format_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            // if (isspace(m_pattern[n]))
            // {
            //     break;
            // }
            if (!format_status &&
                (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                 m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }

            if (format_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    format_status = 1;
                    format_begin = n;
                    ++n;
                    continue;
                }
            }
            if (format_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(format_begin + 1,
                                           n - format_begin - 1);
                    format_status = 0;
                    ++n;
                    break;
                }
            }

            ++n;
            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if (format_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }

            // str = m_pattern.substr(i + 1, n - i - 1);

            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        if (format_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - "
                      << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    // std::cout << "formatter 2" << std::endl;

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
    }

    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string &str)>>
        s_format_items = {
#define XX(str, C)                                                             \
    {                                                                          \
#str,                                                                  \
            [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),     // 消息
            XX(p, LevelFormatItem),       // 日志级别
            XX(r, ElapseFormatItem),      // 累计毫秒数
            XX(c, NameFormatItem),        // 日志名称
            XX(t, ThreadIdFormatItem),    // 线程id
            XX(F, FiberIdFormatItem),     // 协程id
            XX(N, ThreadNameFormatItem),  // 线程名称
            XX(d, DateTimeFormatItem),    // 日期
            XX(f, FileNameFormatItem),    // 文件名称
            XX(l, LineFormatItem),        // 行号
            XX(n, NewLineFormatItem),     // 回车换行
            XX(T, TabFormatItem),         // tab

#undef XX
        };

    for (auto &i : vec) {
        if (std::get<2>(i) == 0) {
            m_items.push_back(
                FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(
                    "<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ")
        // - (" << std::get<2>(i) << ")" << std::endl;
    }
}

/***************
LoggerManager
***************/

/**
 * @brief Construct a new Logger Manager:: Logger Manager object
 *
 * 日志管理器构造函数
 * 构造默认 m_root
 */
LoggerManager::LoggerManager() {
    // 默认的 root logger
    m_root.reset(new Logger);
    // 默认输出到终端 默认 logger 的格式
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    // 添加到 logger map
    m_loggers[m_root->getName()] = m_root;

    // init();
}

/**
 * @brief 找 logger 日志器
 *
 * 没有找到 logger 则创建
 *
 * @param name 日志器名称
 * @return Logger::ptr 日志器指针
 */
Logger::ptr LoggerManager::getLogger(const std::string &name) {
    MutexType::Lock lock(m_mutex);

    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }

    // 没有找到，创建新的名为 name 的 logger
    Logger::ptr logger(new Logger(name));

    // 友元类直接设置 logger 的默认 m_root
    logger->m_root = m_root;
    m_loggers[name] = logger;

    // return it == m_loggers.end() ? m_root : it->second;
    return logger;
}

/**
 * @brief 转 yaml 字符串
 *
 * @return std::string
 */
std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);

    YAML::Node node;
    for (auto &i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

/***************
日志配置
***************/

struct LogAppenderDefine {
    int type = 0;  // 1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine &other) const {
        return type == other.type && level == other.level &&
               formatter == other.formatter && file == other.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &other) const {
        return name == other.name && level == other.level &&
               formatter == other.formatter && appenders == other.appenders;
    }

    bool operator<(const LogDefine &other) const { return name < other.name; }
};

/**
 * @brief string -> log
 *
 * @tparam
 */
template <>
class LexicalCast<std::string, std::set<LogDefine>> {
public:
    std::set<LogDefine> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;

        std::stringstream ss;
        for (size_t i = 0; i < node.size(); i++) {
            const auto &n = node[i];
            if (!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name = n["name"].as<std::string>();

            ld.level = LogLevel::FromString(
                n["level"].IsDefined() ? n["level"].as<std::string>() : "");

            if (n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if (n["appenders"].IsDefined()) {
                for (size_t j = 0; j < n["appenders"].size(); ++j) {
                    auto a = n["appenders"][j];

                    if (!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, "
                                  << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();

                    LogAppenderDefine lad;
                    if (type == "FileLogAppender") {
                        lad.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file "
                                         "is null, "
                                      << a << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }

                    } else if (type == "StdoutLogAppender") {
                        lad.type = 2;

                    } else {
                        std::cout
                            << "log config error: appender type is invalid, "
                            << a << std::endl;
                        continue;
                    }

                    ld.appenders.push_back(lad);
                }
            }

            vec.insert(ld);
        }
        return vec;
    }
};

/**
 * @brief log -> string
 *
 * @tparam
 */
template <>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator()(const std::set<LogDefine> &v) {
        YAML::Node node;
        for (auto &i : v) {
            YAML::Node n;

            n["name"] = i.name;

            if (i.level != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level);
            }

            if (!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for (auto &a : i.appenders) {
                YAML::Node na;
                if (a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if (a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }

                if (a.level != LogLevel::UNKNOW) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if (!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
        // for (auto &i : v)
        // {
        //     node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        // }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 配置
 */
ljrserver::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    ljrserver::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

/**
 * @brief main 函数之前 init 配置
 *
 * 全局变量的构造函数会在 main 函数之前构造
 */
struct LogIniter {
    /**
     * @brief 构造函数
     *
     * 日志配置变更事件监听
     */
    LogIniter() {
        // 添加监听函数
        g_log_defines->addListener([](const std::set<LogDefine> &old_value,
                                      const std::set<LogDefine> &new_value) {
            // 日志配置更改
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
                << "on_logger_config_changed";

            // 遍历新的 logger 配置
            for (auto &new_i : new_value) {
                auto it = old_value.find(new_i);

                ljrserver::Logger::ptr logger;
                if (it == old_value.end()) {
                    // 新增 logger
                    // logger.reset(new ljrserver::Logger(i.name));
                    logger = LJRSERVER_LOG_NAME(new_i.name);
                } else {
                    if (!(new_i == *it)) {
                        // 修改 logger
                        logger = LJRSERVER_LOG_NAME(new_i.name);
                    }
                }

                // 日志器等级
                logger->setLevel(new_i.level);

                // 日志器格式
                if (!new_i.formatter.empty()) {
                    logger->setFormatter(new_i.formatter);
                }

                // 日志器 appender
                logger->clearAppenders();
                for (auto &a : new_i.appenders) {
                    ljrserver::LogAppender::ptr ap;
                    if (a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if (a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                    }
                    // appender 等级
                    ap->setLevel(a.level);

                    // appender 格式
                    if (!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if (!fmt->isError()) {
                            // 格式无误
                            ap->setFormatter(fmt);
                        } else {
                            // 格式错误
                            std::cout << "log.name = " << new_i.name
                                      << "appender type = " << a.type
                                      << " formatter = " << a.formatter
                                      << " is invalid" << std::endl;
                        }
                    }

                    // appender 加入 logger 日志器中
                    logger->addAppender(ap);
                }

                // 删除 logger
                for (auto &old_i : old_value) {
                    auto it = new_value.find(old_i);
                    if (it == new_value.end()) {
                        auto logger = LJRSERVER_LOG_NAME(old_i.name);
                        // 设置大级别
                        logger->setLevel((LogLevel::Level)100);
                        // 清空 appender
                        logger->clearAppenders();
                    }
                }
                // 软删除
            }
        });
        // main 函数之前开启监听
    }
};

/**
 * @brief main 之前执行全局变量的构造函数
 */
static LogIniter __log_init;

// void LoggerManager::init() {}
}  // namespace ljrserver
