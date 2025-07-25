#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    //整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数(只初始化一次)
    MprpcApplication::Init(argc, argv);

    /*演示调用远程发布的rpc方法Login*/
    //底层是调用：RpcChannel->RpcChannel::callMethod  集中来做所有rpc方法调用的参数序列化和网络发送
    //不管做什么操作，最终调用的都是在创建stub对象时传入的RpcChannel的CallMethod方法(相当于中继)
    //所有通过桩类(Stub)调用的Rpc方法最终都转到RpcChannel的callMethod方法上

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhao tianyu");
    request.set_pwd("123456");

    //rpc方法的响应
    fixbug::LoginResponse response;

    //发起rpc方法的调用，这是一个同步的rpc调用过程    MprpcChannel::callMethod
    stub.Login(nullptr, &request, &response, nullptr);       
    
    //一次rpc调用完成，读调用的结果
    if (0 == response.result().errcode())
    {
        std::cout << "rpc login response success: " << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
    }

    /*演示调用远程发布的rpc方法Register*/
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse rsp;

    //以同步的方式发起rpc Register调用请求，等待调用的结果
    stub.Register(nullptr, &req, &rsp, nullptr);

    //一次rpc Register调用完成，读调用的结果
    if (0 == rsp.result().errcode())
    {
        std::cout << "rpc register response success: " << rsp.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error: " << rsp.result().errmsg() << std::endl;
    }

    return 0;
}