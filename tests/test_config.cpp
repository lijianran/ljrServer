
#include "../ljrServer/config.h"
#include "../ljrServer/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

// int port
ljrserver::ConfigVar<int>::ptr g_int_value_config =
    ljrserver::Config::Lookup("system.port", (int)8080, "system port");

// float
ljrserver::ConfigVar<float>::ptr g_float_value_config =
    ljrserver::Config::Lookup("system.value", (float)80.80, "system value");

// float port
// ljrserver::ConfigVar<float>::ptr g_int_valuex_config =
//     ljrserver::Config::Lookup("system.port", (float)8080, "system port");

// vector
ljrserver::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config =
    ljrserver::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                              "system int vec");

// list
ljrserver::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    ljrserver::Config::Lookup("system.int_list", std::list<int>{1, 2},
                              "system int list");

// set
ljrserver::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    ljrserver::Config::Lookup("system.int_set", std::set<int>{1, 2},
                              "system int set");

// unordered_set
ljrserver::ConfigVar<std::unordered_set<int>>::ptr
    g_int_unordered_set_value_config = ljrserver::Config::Lookup(
        "system.int_unordered_set", std::unordered_set<int>{1, 2},
        "system int unordered_set");

// map
ljrserver::ConfigVar<std::map<std::string, int>>::ptr
    g_str_int_map_value_config = ljrserver::Config::Lookup(
        "system.str_int_map", std::map<std::string, int>{{"k", 2}},
        "system str int map");

// unordered_map
ljrserver::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config = ljrserver::Config::Lookup(
        "system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}},
        "system str int unordered_map");

/*************************
 * 测试一 -- yaml
 *************************/

/**
 * @brief 递归打印 yaml
 *
 * @param node
 * @param level
 */
void print_yaml(const YAML::Node &node, int level) {
    if (node.IsScalar()) {
        // 标量
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
            << std::string(level * 4, ' ') << node.Scalar() << " - "
            << node.Type() << " - level" << level;

    } else if (node.IsNull()) {
        // 空 null
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
            << std::string(level * 4, ' ') << "NULL - " << node.Type()
            << " - level" << level;

    } else if (node.IsMap()) {
        // map
        for (auto it = node.begin(); it != node.end(); ++it) {
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
                << std::string(level * 4, ' ') << it->first << " - "
                << it->second.Type() << " - level" << level;

            // 递归打印
            print_yaml(it->second, level + 1);
        }

    } else if (node.IsSequence()) {
        // 列表
        for (size_t i = 0; i < node.size(); i++) {
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
                << std::string(level * 4, ' ') << i << " - " << node[i].Type()
                << " - level" << level;

            // 递归打印
            print_yaml(node[i], level + 1);
        }
    }
}

/**
 * @brief 测试加载 .yml 配置文件
 *
 */
void test_load_yaml() {
    YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/test.yml");
    print_yaml(root, 0);

    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << root;
}

/*************************
 * 测试二 -- 配置文件
 *************************/

/**
 * @brief 测试配置模块加载配置文件
 *
 */
void test_config_load_from_file() {
    // before int
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "before: " << g_int_value_config->getValue();
    // before float
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "before: " << g_float_value_config->toString();
    std::cout << "--------" << std::endl;

// scalar
#define XX(g_var, name, prefix)                             \
    {                                                       \
        auto v = g_var->getValue();                         \
        for (auto &i : v) {                                 \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())        \
                << #prefix " " #name ": " << i;             \
        }                                                   \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix \
            " " #name " yaml:\n" << g_var->toString();      \
        std::cout << "--------" << std::endl;               \
    }

// map
#define XX_M(g_var, name, prefix)                                          \
    {                                                                      \
        auto v = g_var->getValue();                                        \
        for (auto &i : v) {                                                \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())                       \
                << #prefix " " #name ": {" << i.first << " - " << i.second \
                << "}";                                                    \
        }                                                                  \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix                \
            " " #name " yaml:\n" << g_var->toString();                     \
        std::cout << "--------" << std::endl;                              \
    }

    // auto v = g_int_vector_value_config->getValue();
    // for (auto &i : v)
    // {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before int vector: " <<
    //     i;
    // }

    XX(g_int_vector_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    // 加载配置文件
    YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/test.yml");
    ljrserver::Config::LoadFromYaml(root);

    // after int
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "after: " << g_int_value_config->getValue();
    // after float
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "after: " << g_float_value_config->toString();
    std::cout << "--------" << std::endl;

    // v = g_int_vector_value_config->getValue();
    // for (auto &i : v)
    // {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after int vector: " <<
    //     i;
    // }

    XX(g_int_vector_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_unordered_set, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);

#undef XX_M

#undef XX
}

/*************************
 * 测试三 -- 自定义配置类型
 *************************/

/**
 * @brief 自定义的配置类型
 *
 */
class Person {
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

namespace ljrserver {

// 特化
/**
 * @brief string -> Person
 *
 * @tparam
 */
template <>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

/**
 * @brief Person -> string
 *
 * @tparam
 */
template <>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person &p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}  // namespace ljrserver

// 自定义类型的配置项
ljrserver::ConfigVar<Person>::ptr g_person =
    ljrserver::Config::Lookup("class.person", Person(), "class person");

ljrserver::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    ljrserver::Config::Lookup("class.map", std::map<std::string, Person>(),
                              "class person map");

ljrserver::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr
    g_person_vec_map = ljrserver::Config::Lookup(
        "class.vec_map", std::map<std::string, std::vector<Person>>(),
        "class person vector map");

/**
 * @brief 测试自定义的配置类型
 *
 * 测试配置变更监听函数
 */
void test_class() {
    // person
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "before:" << g_person->getValue().toString() << " - yaml:\n"
        << g_person->toString();
    std::cout << "--------" << std::endl;

// map
#define XX_PM(g_var, prefix)                                          \
    {                                                                 \
        auto m = g_var->getValue();                                   \
        for (auto &i : m) {                                           \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())                  \
                << prefix << i.first << " - " << i.second.toString(); \
        }                                                             \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())                      \
            << prefix << "size = " << m.size();                       \
    }

    // 测试配置变更监听函数
    g_person->addListener([](const Person &old_value, const Person &new_value) {
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
            << "监听函数: "
            << "old_value: " << old_value.toString()
            << " new_value: " << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before: ");
    std::cout << "--------" << std::endl;

    // vector map
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "vector map before: " << g_person_vec_map->toString();
    std::cout << "--------" << std::endl;

    // 加载配置文件
    std::cout << "加载配置文件..." << std::endl;
    YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/test.yml");
    ljrserver::Config::LoadFromYaml(root);

    // after person
    std::cout << "--------" << std::endl;
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
        << "after: " << g_person->getValue().toString() << " - yaml:\n"
        << g_person->toString();
    std::cout << "--------" << std::endl;

    // after map
    XX_PM(g_person_map, "class.map after: ");
    std::cout << "--------" << std::endl;

    // after vector map
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "vector map after:\n"
                                             << g_person_vec_map->toString();

#undef XX_PM
}

/*************************
 * 测试四 -- 日志配置
 *************************/

/**
 * @brief 测试日志配置文件
 *
 */
void test_log() {
    static ljrserver::Logger::ptr system_log = LJRSERVER_LOG_NAME("system");
    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    std::cout << ljrserver::LoggerMgr::GetInstance()->toYamlString()
              << std::endl;

    // 加载配置文件
    std::cout << "加载配置文件..." << std::endl;
    YAML::Node root = YAML::LoadFile("/ljrServer/bin/conf/log.yml");
    ljrserver::Config::LoadFromYaml(root);

    std::cout << "============" << std::endl;

    std::cout << ljrserver::LoggerMgr::GetInstance()->toYamlString()
              << std::endl;

    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    // 测试设置日志器 logger 的格式 会自动设置 appender 的输出格式
    system_log->setFormatter("%d - %m %n");
    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
}

/**
 * @brief 测试
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char const *argv[]) {
    // test int
    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) <<
    // g_int_value_config->getValue();

    // test float
    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
    //     << g_float_value_config->toString();

    // 测试加载 .yml 配置文件
    // test_load_yaml();

    // 测试配置模块加载配置文件
    // test_config_load_from_file();

    // 测试自定义的配置类型 测试配置变更监听函数
    // test_class();

    // 测试日志配置文件
    test_log();

    // 测试 Visit
    // ljrserver::Config::Visit([](ljrserver::ConfigVarBase::ptr var) {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT())
    //         << "name=" << var->getName()
    //         << " description=" << var->getDescription()
    //         << " typename=" << var->getTypeName()
    //         << " value=" << var->toString();
    // });

    return 0;
}
