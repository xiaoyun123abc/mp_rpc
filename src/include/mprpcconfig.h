#pragma once

#include <unordered_map>
#include <string>

//框架读取配置文件类
//rpcserver_ip   rpcserver_port  zookeeper_ip  zookeeper_port
class MprpcConfig
{
public:
    //负责解析加载配置文件
    void LoadConfigFile(const char *config_file);

    //查询配置项信息
    std::string Load(std::string key);

private:
    //map表  =>  存储配置信息
    std::unordered_map<std::string, std::string> m_configMap;

    //去掉字符串前后的空格
    void Trim(std::string &src_buf);
};