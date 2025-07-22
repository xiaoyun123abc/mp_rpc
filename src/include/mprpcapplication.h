#pragma once

#include "mprpcconfig.h"

//mprpc框架的基础类，负责框架的一些初始化操作
class MprpcApplication
{
public:
    //框架的初始化操作，获取配置文件路径并调用配置文件对象进行加载配置参数并保存
    static void Init(int argc, char **argv);

    //获取唯一实例对象
    static MprpcApplication& GetInstance();

    //获取配置信息
    static MprpcConfig& GetConfig();

private:
    //存储配置信息,只需要从磁盘中获取一次即可
    static MprpcConfig m_config;        

    //构造函数
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
};