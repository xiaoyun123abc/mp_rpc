#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
// 这是一个单独的线程，在zkserver真的连接成功时回调
void global_watcher(zhandle_t *zh, int type,
                    int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)          // 回调的消息类型，是和会话相关的消息类型(连接或断开连接)
    {
        if (state == ZOO_CONNECTED_STATE)   // zkclient和zkserver连接成功
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh);       //从指定的句柄上获取此信号量
            sem_post(sem);                                  //给信号量的资源+1
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{

}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle);     //关闭句柄，释放资源
    }
}

//zkclient启动连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    //30000：会话的超时时间
    //返回的就是句柄
    /*
    zookeeper_mt: 多线程版本
    zookeeper的API客户端程序提供了三个线程：
    (1)API线程
    (2)网络I/O线程          pthread_create重新起了一个线程做网络I/O  poll
    (3)watcher回调线程      pthread_create
    */
    //global_watcher:回调
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle)
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    //创建信号量并初始化为零
    sem_t sem;
    sem_init(&sem, 0, 0);

    //设置上下文(给指定的句柄添加一些额外的信息)，给句柄绑定信号量，然后等待信号量
    zoo_set_context(m_zhandle, &sem);

    //因为初始化信号量为零值，获取不到资源，主线程等待(阻塞)
    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;        //等zookeeper响应后sem加1，则成功
}

//在zkserver上根据指定的path创建node节点，state默认0是代表永久性节点
void ZkClient::Create(const char *path, const char *data, int dataLen, int state)
{
    char path_buffer[128];    
    int bufferLen = sizeof(path_buffer);
    int flag;

    //先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (ZNONODE == flag)        //表示path的节点不存在
    {
        //创建指定path的znode节点
        flag = zoo_create(m_zhandle, path, data, dataLen,
                          &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferLen);
        if (flag == ZOK)
        {
            std::cout << "znode create success... path:" << path << std::endl;
        }
        else
        {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error... path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

//传入参数，根据参数指定的znode节点路径，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "get znode error... path:" << path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}