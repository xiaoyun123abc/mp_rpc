#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

int main(int argc, char **argv)
{
    //整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数(只初始化一次)
    MprpcApplication::Init(argc, argv);

    /*演示调用远程发布的rpc方法Login*/
    //底层是调用：RpcChannel->RpcChannel::callMethod  集中来做所有rpc方法调用的参数序列化和网络发送
    //不管做什么操作，最终调用的都是在创建stub对象时传入的RpcChannel的CallMethod方法(相当于中继)
    //所有通过桩类(Stub)调用的Rpc方法最终都转到RpcChannel的callMethod方法上

    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    
    //rpc方法的请求参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);

    //rpc方法的响应
    fixbug::GetFriendsListResponse response;

    //定义一个控制对象(保存rpc调用过程中的状态信息)
    MprpcController controller;

    //发起rpc方法的调用，这是一个同步的rpc调用过程    MprpcChannel::callMethod
    stub.GetFriendsList(&controller, &request, &response, nullptr);       
    
    //一次rpc调用完成，读调用的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;       //rpc调用过程中出错
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.friends_size();
            for (int i = 0; i < size; i++)
            {
                std::cout << "index: " << (i+1) << " name: " << response.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendsList response error: " << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}