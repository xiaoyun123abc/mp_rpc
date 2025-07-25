#pragma once

#include "google/protobuf/service.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include "zookeeperutil.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <google/protobuf/descriptor.h>


//框架提供的专门发布rpc服务的网络对象类
class RpcProvider
{
public:
    //框架开发时要考虑到：不能依赖于某个具体的业务，要可以任意的service
    //这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    //启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();

private:
    //组合EventLoop
    muduo::net::EventLoop m_eventLoop;

    //service服务类型信息
    struct ServiceInfo
    {
        //保存服务对象
        google::protobuf::Service *m_service; 
        //保存服务方法      
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;
    };
    //存储注册成功的服务对象(eg:UserServiceRpc)   及其   服务方法(eg:Login)的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    //新的socket连接回调方法
    void OnConnection(const muduo::net::TcpConnectionPtr&);

    //已建立用户的读写事件回调
    //muduo库如果发现有可读写的消息发生后，就会给上层主动调用此回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    //Closure的回调操作, 用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};