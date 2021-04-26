
#include "../ljrServer/config.h"
#include "../ljrServer/log.h"
#include <yaml-cpp/yaml.h>

ljrserver::ConfigVar<int>::ptr g_int_value_config = ljrserver::Config::Lookup("system.port", (int)8080, "system port");
ljrserver::ConfigVar<float>::ptr g_float_value_config = ljrserver::Config::Lookup("system.value", (float)80.80, "system value");

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
    YAML::Node root = YAML::LoadFile("bin/conf/log.yml");
    print_yaml(root, 0);

    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << root;
}

void test_config()
{
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    YAML::Node root = YAML::LoadFile("bin/conf/log.yml");
    ljrserver::Config::LoadFromYmal(root);
    

    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << "after: " << g_float_value_config->toString();

}

int main(int argc, char const *argv[])
{

    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << g_int_value_config->getValue();
    // LJRSERVER_LOG_INFO(LJRSERVER_LOG_ROOT()) << g_float_value_config->toString();
    // test_yaml();

    test_config();

    return 0;
}
