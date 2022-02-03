
#include "config.h"

namespace ljrserver {

// static 静态变量需要声明定义
// Config::ConfigVarMap Config::s_datas;

/**
 * @brief 查找配置参数，返回配置参数的基类
 *
 * @param name 配置项名称
 * @return ConfigVarBase::ptr
 */
ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    // 上读锁
    RWMutexType::ReadLock lock(GetMutex());

    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * @brief 访问所有节点
 *
 * @param prefix 前缀
 * @param node 当前节点
 * @param output 配置列表
 */
static void ListAllMember(
    const std::string &prefix, const YAML::Node &node,
    std::list<std::pair<std::string, const YAML::Node>> &output) {
    // 检查前缀
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
        std::string::npos) {
        // 名称错误
        LJRSERVER_LOG_ERROR(LJRSERVER_LOG_ROOT())
            << "Config invalid name: " << prefix << " : " << node;
        return;
    }

    // 加入配置列表
    output.push_back(std::make_pair(prefix, node));

    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            // 递归访问
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, output);
        }
    }
}

/**
 * @brief 使用 YAML::Node 初始化配置模块
 *
 *  YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/test.yml");
    ljrserver::Config::LoadFromYaml(root);
    
 * @param root
 */
void Config::LoadFromYaml(const YAML::Node &root) {
    // 配置列表
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    // 遍历配置
    ListAllMember("", root, all_nodes);

    for (auto &i : all_nodes) {
        // 配置名称
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        // 转小写
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        // 查找基类
        ConfigVarBase::ptr var = LookupBase(key);

        // 有定义该配置项
        if (var) {
            if (i.second.IsScalar()) {
                // 标量
                var->fromString(i.second.Scalar());
            } else {
                // 向量
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

/**
 * @brief 遍历配置模块里所有的配置项
 *
 * @param cb 访问函数
 */
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    // 上读锁
    RWMutexType::ReadLock lock(GetMutex());
    // 获取配置项 map
    ConfigVarMap &m = GetDatas();

    for (auto it = m.begin(); it != m.end(); ++it) {
        // 传入 ConfigVarBase 执行
        cb(it->second);
    }
}

}  // namespace ljrserver