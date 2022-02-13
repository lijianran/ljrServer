/**
 * @file application.cpp
 * @author lijianran (lijianran@outlook.com)
 * @brief 应用
 * @version 0.1
 * @date 2022-02-13
 */

#include "application.h"
#include "daemon.h"
#include "config.h"
#include "log.h"
#include "env.h"
#include "iomanager.h"

namespace ljrserver {

// system 日志
static ljrserver::Logger::ptr g_logger = LJRSERVER_LOG_NAME("system");

// 配置 工作路径
static ljrserver::ConfigVar<std::string>::ptr g_server_work_path =
    ljrserver::Config::Lookup(
        "server.work_path", std::string("/root/ljrServer"), "server work path");

// 配置 服务器 pid
static ljrserver::ConfigVar<std::string>::ptr g_server_pid_file =
    ljrserver::Config::Lookup("server.pid_file", std::string("ljrserver.pid"),
                              "server pid file");

/**
 * @brief http 服务器配置
 *
 */
struct HttpServerConf {
    std::vector<std::string> address;  // 地址列表
    int keepalive = 0;                 // 非长连接
    int timeout = 1000 * 2 * 60;       // 2 min
    std::string name;                  // 名称

    bool isValid() const { return !address.empty(); }

    bool operator==(const HttpServerConf& oth) const {
        return address == oth.address && keepalive == oth.keepalive &&
               timeout == oth.timeout && name == oth.name;
    }
};

template <>
class LexicalCast<std::string, HttpServerConf> {
public:
    HttpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        if (node["address"].IsDefined()) {
            for (size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template <>
class LexicalCast<HttpServerConf, std::string> {
public:
    std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["name"] = conf.name;
        for (auto& addr : conf.address) {
            node["address"].push_back(addr);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// 配置 服务器配置属性
static ljrserver::ConfigVar<std::vector<HttpServerConf>>::ptr
    g_http_servers_conf = ljrserver::Config::Lookup(
        "http_servers", std::vector<HttpServerConf>(), "http servers config");

// 类中的静态成员需要在类外定义
Application* Application::s_instance = nullptr;

/**
 * @brief 应用构造函数
 *
 */
Application::Application() {
    // 应用实例
    s_instance = this;
}

/**
 * @brief 初始化
 *
 * @param argc
 * @param argv
 * @return true
 * @return false
 */
bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    // 增加启动参数描述
    ljrserver::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    ljrserver::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    ljrserver::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    ljrserver::EnvMgr::GetInstance()->addHelp("p", "print help");

    // 解析启动参数
    bool is_print_help = false;
    if (!ljrserver::EnvMgr::GetInstance()->init(argc, argv)) {
        is_print_help = true;
    }

    if (ljrserver::EnvMgr::GetInstance()->has("p")) {
        is_print_help = true;
    }

    // 加载配置
    std::string conf_path = ljrserver::EnvMgr::GetInstance()->getConfigPath();
    LJRSERVER_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    ljrserver::Config::LoadFromConfDir(conf_path);

    // 加载模块
    // ModuleMgr::GetInstance()->init();
    // std::vector<Module::ptr> modules;
    // ModuleMgr::GetInstance()->listAll(modules);

    // for (auto i : modules) {
    //     i->onBeforeArgsParse(argc, argv);
    // }

    if (is_print_help) {
        ljrserver::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    // for (auto i : modules) {
    //     i->onAfterArgsParse(argc, argv);
    // }
    // modules.clear();

    // 普通进程及守护进程二选一 全有仍为守护进程
    int run_type = 0;
    if (ljrserver::EnvMgr::GetInstance()->has("s")) {
        run_type = 1;
    }
    if (ljrserver::EnvMgr::GetInstance()->has("d")) {
        run_type = 2;
    }

    if (run_type == 0) {
        // 必须二选一
        ljrserver::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    // 检测不要重复启动服务器
    std::string pidfile =
        g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
    if (ljrserver::FSUtil::IsRunningPidfile(pidfile)) {
        LJRSERVER_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }

    // 创建工作目录 日志等
    if (!ljrserver::FSUtil::Mkdir(g_server_work_path->getValue())) {
        LJRSERVER_LOG_FATAL(g_logger)
            << "create work path [" << g_server_work_path->getValue()
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 初始化成功
    return true;
}

/**
 * @brief 启动应用
 *
 * @return true
 * @return false
 */
bool Application::run() {
    // 是否守护进程
    bool is_daemon = ljrserver::EnvMgr::GetInstance()->has("d");
    // 执行
    return start_daemon(m_argc, m_argv,
                        std::bind(&Application::main, this,
                                  std::placeholders::_1, std::placeholders::_2),
                        is_daemon);
}

/**
 * @brief 进程主函数
 *
 * @param argc
 * @param argv
 * @return int
 */
int Application::main(int argc, char** argv) {
    LJRSERVER_LOG_INFO(g_logger) << "main";

    {
        std::string pidfile = g_server_work_path->getValue() + "/" +
                              g_server_pid_file->getValue();
        std::ofstream ofs(pidfile);
        if (!ofs) {
            LJRSERVER_LOG_ERROR(g_logger)
                << "open pidfile " << pidfile << " failed";
            return false;
        }
        // 输出进程 id 到文件 ljrserver.pid
        ofs << getpid();
    }

    // auto http_confs = g_http_servers_conf->getValue();
    // for (auto& conf : http_confs) {
    //     LJRSERVER_LOG_INFO(g_logger)
    //         << std::endl
    //         << LexicalCast<HttpServerConf, std::string>()(conf);
    // }

    // IO 调度器
    ljrserver::IOManager iom(1);
    // 调度任务
    iom.schedule(std::bind(&Application::run_fiber, this));
    // 停止调度
    // iom.stop();
    // 程序结束
    return 0;
}

/**
 * @brief 任务协程
 *
 * @return int
 */
int Application::run_fiber() {
    // http 服务器配置
    auto http_confs = g_http_servers_conf->getValue();
    for (auto& conf : http_confs) {
        // 打印配置
        LJRSERVER_LOG_INFO(g_logger)
            << LexicalCast<HttpServerConf, std::string>()(conf);

        // 构建成功的地址
        std::vector<Address::ptr> address;
        for (auto& addr : conf.address) {
            // 查找 :
            size_t pos = addr.find(":");
            if (pos == std::string::npos) {
                // 找不到 : 端口
                LJRSERVER_LOG_ERROR(g_logger) << "invalid address: " << addr;
                // address.push_back(UnixAddress::ptr(new UnixAddress(addr)));
                continue;
            }
            // 端口号
            int32_t port = atoi(addr.substr(pos + 1).c_str());

            // 127.0.0.1
            auto ipaddr =
                ljrserver::IPAddress::Create(addr.substr(0, pos).c_str(), port);
            if (ipaddr) {
                // 成功
                address.push_back(ipaddr);
                continue;
            }

            // 本机网卡地址
            std::vector<std::pair<Address::ptr, uint32_t>> result;
            if (ljrserver::Address::GetInterfaceAddresses(
                    result, addr.substr(0, pos))) {
                // 遍历网卡
                for (auto& x : result) {
                    // 获取地址
                    auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                    if (ipaddr) {
                        ipaddr->setPort(port);
                    }
                    address.push_back(ipaddr);
                }
                // 成功
                continue;
            }

            // 找其他地址
            auto other_addr = ljrserver::Address::LookupAny(addr);
            if (other_addr) {
                // 成功
                address.push_back(other_addr);
                continue;
            }

            // 构建配置地址失败
            LJRSERVER_LOG_ERROR(g_logger) << "invalid address: " << addr;
            _exit(0);
        }

        // http 服务器
        ljrserver::http::HttpServer::ptr server(
            new ljrserver::http::HttpServer(conf.keepalive));
        // bind 失败的地址
        std::vector<Address::ptr> fails;
        if (!server->bind(address, fails)) {
            // bind 失败
            for (auto& addr : fails) {
                // 打印失败的地址
                LJRSERVER_LOG_ERROR(g_logger) << "bind address fail:" << *addr;
            }
            _exit(0);
        }

        // 启动服务器
        server->start();
        m_httpservers.push_back(server);
    }
    return 0;
}

};  // namespace ljrserver