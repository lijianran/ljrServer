
#include "log.h"
#include <iostream>
#include <functional>
#include <time.h>
#include "config.h"

namespace ljrserver
{
    /*
    FormatItem 类
    // %m -- 消息体
    // %p -- level
    // %r -- 启动后时间
    // %c -- 日志名称
    // %t -- 线程id
    // %n -- 回车换行
    // %d -- 时间
    // %f -- 文件名
    // %l -- 行号
    */
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            // os << logger->getName();
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y.%m.%d %H:%M:%S") : m_format(format)
        {
            // 为什么？
            if (m_format.empty())
            {
                m_format = "%Y.%m.%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
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

    class FileNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str = "") : m_string(str) {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    /***************
    LogEvent
    ***************/
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       const char *file, int32_t line, uint32_t elapse,
                       uint32_t thread_id, uint32_t fiber_id, uint64_t time)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id),
          m_fiberID(fiber_id), m_time(time), m_logger(logger), m_level(level)
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buffer = nullptr;
        int len = vasprintf(&buffer, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buffer, len);
            free(buffer);
        }
    }

    /***************
    LogEventWrap
    ***************/
    LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    LogEvent::ptr LogEventWrap::getEvent() const
    {
        return m_event;
    }

    /***************
    LogAppender
    ***************/
    void LogAppender::setFormatter(LogFormatter::ptr formatter)
    {
        m_formatter = formatter;
        if (m_formatter)
        {
            m_hasFormatter = true;
        }
        else
        {
            m_hasFormatter = false;
        }
    }

    /***************
    LogLevel
    ***************/
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
            break;
        }

        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
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
    Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG)
    {
        // 默认格式
        m_formatter.reset(new LogFormatter("%d{%Y.%m.%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

    std::string Logger::toYamlString()
    {
        YAML::Node node;
        node["name"] = m_name;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }

        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogFormatter::ptr Logger::getFormatter()
    {
        return m_formatter;
    }

    void Logger::setFormatter(LogFormatter::ptr val)
    {
        m_formatter = val;

        for (auto &i : m_appenders)
        {
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &val)
    {
        ljrserver::LogFormatter::ptr new_val(new ljrserver::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter error name = " << m_name
                      << " value = " << val << " invalid formatter" << std::endl;
            return;
        }

        // m_formatter = new_val;
        setFormatter(new_val);
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        // 少了!
        if (!appender->getFormatter())
        {
            // 如果appender没有定义格式，将logger的默认格式传给appender
            appender->m_formatter = m_formatter;
            // appender->setFormatter(m_formatter);
        }

        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        m_appenders.clear();
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            if (!m_appenders.empty())
            {
                for (auto &i : m_appenders)
                {
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    /***************
    LogAppender - stdout
    ***************/
    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /***************
    LogAppender - file
    ***************/
    FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename)
    {
        reopen();
    }

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            // bool test = !!m_filestream;
            // std::cout << test;
            std::string str = m_formatter->format(logger, level, event);
            m_filestream << str;
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }

        m_filestream.open(m_filename, std::ofstream::app);
        return !!m_filestream;
    }

    /***************
    LogFormatter
    ***************/
    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    void LogFormatter::init()
    {
        // std::cout << "formatter init" << std::endl;
        // str, format, type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int format_status = 0;
            size_t format_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size())
            {
                // if (isspace(m_pattern[n]))
                // {
                //     break;
                // }
                if (!format_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }

                if (format_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        format_status = 1;
                        format_begin = n;
                        ++n;
                        continue;
                    }
                }
                if (format_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(format_begin + 1, n - format_begin - 1);
                        format_status = 0;
                        ++n;
                        break;
                    }
                }

                ++n;
                if (n == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (format_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }

                // str = m_pattern.substr(i + 1, n - i - 1);

                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            if (format_status == 1)
            {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        // std::cout << "formatter 2" << std::endl;

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, std::string(), 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
    {                                                                            \
#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),  // 消息
            XX(p, LevelFormatItem),    // 日志级别
            XX(r, ElapseFormatItem),   // 累计毫秒数
            XX(c, NameFormatItem),     // 日志名称
            XX(t, ThreadIdFormatItem), // 线程id
            XX(F, FiberIdFormatItem),  // 协程id
            XX(n, NewLineFormatItem),  // 回车换行
            XX(d, DateTimeFormatItem), // 日期
            XX(f, FileNameFormatItem), // 文件名称
            XX(l, LineFormatItem),     // 行号
            XX(T, TabFormatItem),      // tab
#undef XX
        };

        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }

            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
    }

    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

        m_loggers[m_root->getName()] = m_root;

        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;

        // return it == m_loggers.end() ? m_root : it->second;
        return logger;
    }

    std::string LoggerManager::toYamlString()
    {
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    struct LogAppenderDefine
    {
        int type = 0; // 1 File, 2 Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &other) const
        {
            return type == other.type && level == other.level && formatter == other.formatter && file == other.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &other) const
        {
            return name == other.name && level == other.level && formatter == other.formatter && appenders == other.appenders;
        }

        bool operator<(const LogDefine &other) const
        {
            return name < other.name;
        }
    };

    // vector
    template <>
    class LexicalCast<std::string, std::set<LogDefine>>
    {
    public:
        std::set<LogDefine> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::set<LogDefine> vec;

            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                const auto &n = node[i];
                if (!n["name"].IsDefined())
                {
                    std::cout << "log config error: name is null, " << n << std::endl;
                    continue;
                }

                LogDefine ld;
                ld.name = n["name"].as<std::string>();
                ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
                if (n["formatter"].IsDefined())
                {
                    ld.formatter = n["formatter"].as<std::string>();
                }
                if (n["appenders"].IsDefined())
                {
                    for (size_t j = 0; j < n["appenders"].size(); ++j)
                    {
                        auto a = n["appenders"][j];

                        if (!a["type"].IsDefined())
                        {
                            std::cout << "log config error: appender type is null, " << a << std::endl;
                            continue;
                        }
                        std::string type = a["type"].as<std::string>();

                        LogAppenderDefine lad;
                        if (type == "FileLogAppender")
                        {
                            lad.type = 1;
                            if (!a["file"].IsDefined())
                            {
                                std::cout << "log config error: fileappender file is null, " << a << std::endl;
                                continue;
                            }
                            lad.file = a["file"].as<std::string>();
                            if (a["formatter"].IsDefined())
                            {
                                lad.formatter = a["formatter"].as<std::string>();
                            }
                        }
                        else if (type == "StdoutLogAppender")
                        {
                            lad.type = 2;
                        }
                        else
                        {
                            std::cout << "log config error: appender type is invalid, " << a << std::endl;
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

    template <>
    class LexicalCast<std::set<LogDefine>, std::string>
    {
    public:
        std::string operator()(const std::set<LogDefine> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                YAML::Node n;
                n["name"] = i.name;
                if (i.level != LogLevel::UNKNOW)
                {
                    n["level"] = LogLevel::ToString(i.level);
                }

                if (!i.formatter.empty())
                {
                    n["formatter"] = i.formatter;
                }
                for (auto &a : i.appenders)
                {
                    YAML::Node na;
                    if (a.type == 1)
                    {
                        na["type"] = "FileLogAppender";
                        na["file"] = a.file;
                    }
                    else if (a.type == 2)
                    {
                        na["type"] = "StdoutLogAppender";
                    }

                    if (a.level != LogLevel::UNKNOW)
                    {
                        na["level"] = LogLevel::ToString(a.level);
                    }

                    if (!a.formatter.empty())
                    {
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

    ljrserver::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
        ljrserver::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    // main函数之前构造
    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine> &old_value,
                                                    const std::set<LogDefine> &new_value) {
                LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "on_logger_conf_changed";
                for (auto &i : new_value)
                {
                    auto it = old_value.find(i);
                    ljrserver::Logger::ptr logger;
                    if (it == old_value.end())
                    {
                        // 新增logger
                        // logger.reset(new ljrserver::Logger(i.name));
                        logger = LJRSERVER_LOG_NAME(i.name);
                    }
                    else
                    {
                        if (!(i == *it))
                        {
                            // 修改logger
                            logger = LJRSERVER_LOG_NAME(i.name);
                        }
                    }
                    logger->setLevel(i.level);
                    if (!i.formatter.empty())
                    {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for (auto &a : i.appenders)
                    {
                        ljrserver::LogAppender::ptr ap;
                        if (a.type == 1)
                        {
                            ap.reset(new FileLogAppender(a.file));
                        }
                        else if (a.type == 2)
                        {
                            ap.reset(new StdoutLogAppender);
                        }
                        ap->setLevel(a.level);

                        if (!a.formatter.empty())
                        {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if (!fmt->isError())
                            {
                                ap->setFormatter(fmt);
                            }
                            else
                            {
                                std::cout << "log.name = " << i.name
                                          << "appender type = " << a.type
                                          << " formatter = " << a.formatter
                                          << " is invalid" << std::endl;
                            }
                        }

                        logger->addAppender(ap);
                    }

                    for (auto &i : old_value)
                    {
                        auto it = new_value.find(i);
                        if (it == new_value.end())
                        {
                            // 删除logger
                            auto logger = LJRSERVER_LOG_NAME(i.name);
                            logger->setLevel((LogLevel::Level)100);
                            logger->clearAppenders();
                        }
                    }
                }
            });
        }
    };

    static LogIniter __log_init;

    void LoggerManager::init()
    {
    }
}
