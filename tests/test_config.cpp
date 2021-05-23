
#include "../ljrServer/config.h"
#include "../ljrServer/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

ljrserver::ConfigVar<int>::ptr g_int_value_config = ljrserver::Config::Lookup("system.port", (int)8080, "system port");

ljrserver::ConfigVar<float>::ptr g_float_value_config = ljrserver::Config::Lookup("system.value", (float)80.80, "system value");

ljrserver::ConfigVar<float>::ptr g_int_valuex_config = ljrserver::Config::Lookup("system.port", (float)8080, "system port");

// vector
ljrserver::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config = ljrserver::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

// list
ljrserver::ConfigVar<std::list<int>>::ptr g_int_list_value_config = ljrserver::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");

// set
ljrserver::ConfigVar<std::set<int>>::ptr g_int_set_value_config = ljrserver::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");

// unordered_set
ljrserver::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config = ljrserver::Config::Lookup("system.int_unordered_set", std::unordered_set<int>{1, 2}, "system int unordered_set");

// map
ljrserver::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config = ljrserver::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");

// unordered_map
ljrserver::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config = ljrserver::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}}, "system str int unordered_map");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); i++)
        {
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("bin/conf/test.yml");
    print_yaml(root, 0);

    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << root;
}

void test_config()
{
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix)                                                                       \
    {                                                                                                 \
        auto v = g_var->getValue();                                                                   \
        for (auto &i : v)                                                                             \
        {                                                                                             \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix " " #name ": " << i;                  \
        }                                                                                             \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix)                                                                                       \
    {                                                                                                                   \
        auto v = g_var->getValue();                                                                                     \
        for (auto &i : v)                                                                                               \
        {                                                                                                               \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        }                                                                                                               \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString();                   \
    }

    // auto v = g_int_vector_value_config->getValue();
    // for (auto &i : v)
    // {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before int vector: " << i;
    // }

    XX(g_int_vector_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("bin/conf/test.yml");
    ljrserver::Config::LoadFromYaml(root);

    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    // v = g_int_vector_value_config->getValue();
    // for (auto &i : v)
    // {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after int vector: " << i;
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

class Person
{
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const
    {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

namespace ljrserver
{

    // vector
    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}

ljrserver::ConfigVar<Person>::ptr g_person = ljrserver::Config::Lookup("class.person", Person(), "class person");

ljrserver::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    ljrserver::Config::Lookup("class.map", std::map<std::string, Person>(), "class person map");

ljrserver::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map =
    ljrserver::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "class person vector map");

void test_class()
{
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before:" << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix)                                                                               \
    {                                                                                                      \
        auto m = g_var->getValue();                                                                        \
        for (auto &i : m)                                                                                  \
        {                                                                                                  \
            LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << prefix << i.first << " - " << i.second.toString(); \
        }                                                                                                  \
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << prefix << ": size = " << m.size();                     \
    }

    g_person->addListener([](const Person &old_value, const Person &new_value) {
        LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "old_value: " << old_value.toString()
                                                 << "new_value: " << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");

    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("bin/conf/test.yml");
    ljrserver::Config::LoadFromYaml(root);

    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();

    XX_PM(g_person_map, "class.map after: ");
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_person_vec_map->toString();

#undef XX_PM
}

void test_log()
{
    static ljrserver::Logger::ptr system_log = LJRSERVER_LOG_NAME("system");
    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    std::cout << ljrserver::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    YAML::Node root = YAML::LoadFile("bin/conf/log.yml");
    ljrserver::Config::LoadFromYaml(root);

    std::cout << "============" << std::endl;

    std::cout << ljrserver::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %m %n");
    LJRSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main(int argc, char const *argv[])
{

    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << g_int_value_config->getValue();

    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << g_float_value_config->toString();
    
    // test_yaml();

    // test_config();

    // test_class();

    // test_log();

    // ljrserver::Config::Visit([](ljrserver::ConfigVarBase::ptr var) {
    //     LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "name = " << var->getName()
    //                                              << " description = " << var->getDescription()
    //                                              << " typename = " << var->getTypeName()
    //                                              << " value = " << var->toString();
    // });

    return 0;
}
