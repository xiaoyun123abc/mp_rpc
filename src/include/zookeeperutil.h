#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

//封装的zk客户端类
class ZkClient
{
public:    
    //构造函数
    ZkClient();
    ~ZkClient();

    //zkclient启动连接zkserver
    void Start();

    //在zkserver上根据指定的path创建node节点，state默认0是代表永久性节点
    void Create(const char *path, const char *data, int dataLen, int state=0);

    //传入参数，根据参数指定的znode节点路径，获取znode节点的值
    std::string GetData(const char *path);

private:
    // zk的客户端句柄，通过句柄就可以去操作zkserver了
    zhandle_t *m_zhandle;
};