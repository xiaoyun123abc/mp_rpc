#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"

class FriendService:public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service! userid: " << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("zhao tianyu");
        vec.push_back("xue ying");
        return vec;
    }

    //重写基类方法
    void GetFriendsList(::google::protobuf::RpcController* controller,
                        const ::fixbug::GetFriendsListRequest* request,       
                        ::fixbug::GetFriendsListResponse* response,           
                        ::google::protobuf::Closure* done)  
    {
        //取出数据
        uint32_t userid = request->userid();

        //做相应业务
        std::vector<std::string> friendsList = GetFriendsList(userid);

        //构造相应
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name : friendsList)
        {
            std::string *p = response->add_friends();
            *p = name;
        }

        done->Run();
    }        
};

int main(int argc, char **argv)
{
    LOG_INFO("first log message");
    LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);

    //调用框架的初始化操作   provider -i config.conf
    //由于此对象方法都是static，故初始化后在其它函数中使用此对象的成员时，都是互通的
    MprpcApplication::Init(argc, argv);

    //provider是一个rpc网络服务对象，将UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService);      //生成了一张map表，存储了用户通过NotifyService,利用我们的框架，发布了一系列rpc服务

    //启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}