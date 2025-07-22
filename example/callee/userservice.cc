#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/
class UserService: public fixbug::UserServiceRpc     //使用在RPC服务发布端(rpc服务提供者)
{
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name" << name << "pwd" << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing loacl service: Register" << std::endl;
        std::cout<< "id: " << id << " name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    //重写基类UserServiceRpc的虚函数，下面这些方法都是框架直接调用的
    /*
    如果你在远端发起一个想调用我这台机器上的UserService对象的Login方法的话，首先，你会发起一个RPC请求到我这儿来，
    会被我们的RPC框架所接收，RPC框架根据你发过来的请求(哪个方法、有哪些参数)，然后帮我们匹配到Login方法，然后它就会帮我们把网络上发起的请求上报上来，
    我们接收到这个请求以后呢，我们会从请求中拿出数据，然后做本地业务，并填写相应的响应，然后再执行一个回调，相当于把执行完的RPC方法的返回值再塞给框架，
    由框架给我们进行数据的序列化，再通过框架的网络把响应返回回去，发送给你。

    1.caller   ===>   Login(LoginRequest)     ===>    muduo    ===>    callee
    2.callee   ===>   Login(LoginRequest)     ===>    交到下面重写的Login方法上了

    框架帮我们从远端接收到rpc请求后，它收到的是rpc的一个描述(方法、请求参数)，通过一系列操作上报到重写的Login方法中，我就可以从请求操作中解析出参数来做本地业务
    填写相应消息，然后执行一个回调即可
    */
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,       //登录请求
                       ::fixbug::LoginResponse* response,           //登录响应
                       ::google::protobuf::Closure* done)           //回调
    {
        //框架给业务上报了请求参数LoginRequest, 应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        //做本地业务
        bool login_result = Login(name, pwd);   

        //将响应写入
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //调用执行回调操作,执行相应对象数据的序列化和网络发送  ==>  都是由框架来完成的
        //在rpcprovider中定义了这个会带哦，用来将响应发送回去
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,       //注册请求
                       ::fixbug::RegisterResponse* response,           //注册响应
                       ::google::protobuf::Closure* done)              //回调
    {
        //从request中取数据
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        //执行业务
        bool ret = Register(id, name, pwd);

        //构建回应
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();        //执行done的回调
    }
};

int main(int argc, char **argv)
{
    //调用框架的初始化操作   provider -i config.conf
    //由于此对象方法都是static，故初始化后在其它函数中使用此对象的成员时，都是互通的
    MprpcApplication::Init(argc, argv);

    //provider是一个rpc网络服务对象，将UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService);      //生成了一张map表，存储了用户通过NotifyService,利用我们的框架，发布了一系列rpc服务

    //启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}