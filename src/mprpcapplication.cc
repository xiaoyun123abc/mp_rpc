#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include <string>

//初始化静态成员变量
MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
    std::cout << "format: command -i <configfile>" << std::endl;

}

//框架初始化操作，获取配置文件路径并调用配置文件对象进行加载配置参数并保存
void MprpcApplication::Init(int argc, char **argv)
{
    //如果给予的参数小于2 => 未能正确输入
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    //解析命令行选项和参数  =>  获取配置文件信息
    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    //开始加载配置文件了,功能解耦->单独写一个.h .cc
    //rpcserver_ip   rpcserver_port  zookeeper_ip  zookeeper_port
    m_config.LoadConfigFile(config_file.c_str());

    // std::cout << "rpcserverip:" << m_config.Load("rpcserverip") << std::endl;
    // std::cout << "rpcserverport:" << m_config.Load("rpcserverport") << std::endl;
    // std::cout << "zookeeperip:" << m_config.Load("zookeeperip") << std::endl;
    // std::cout << "zookeeperport:" << m_config.Load("zookeeperport") << std::endl;
}


//获取唯一实例对象
MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}


//获取配置文件信息
MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}