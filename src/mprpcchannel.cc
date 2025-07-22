#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"

/*
header_size + service_name method_name args_size + args
*/
//所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                             google::protobuf::RpcController* controller, 
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response, 
                             google::protobuf::Closure* done)
{
    /*
    一.ServiceDescriptor:
    ServiceDescriptor 是 Protocol Buffers（Protobuf）中的一个类，用于 描述一个 Protobuf 服务（Service）的元信息，包括：
        (1)服务名称（name()）
        (2)包含的方法（method_count() 和 method(int index)）
        (3)服务的全限定名等

    二.method->service():
    method：
        指向某个 RPC 方法的描述符（MethodDescriptor），例如 UserService.Login。
    service()：
        是该方法的成员函数，返回该方法所属的服务的 ServiceDescriptor。
    */
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();          //service_name   :   UserServiceRpc
    std::string method_name = method->name();       //method_name    :   Login

    //获取参数的序列化字符串长度  args_size
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))      //将request请求序列化成字符串并存入args_str中
    {
        args_size = args_str.size();
    }
    else
    {
        // std::cout << "serialize request error!" << std::endl;
        controller->SetFailed("serialize request error!");
        return;
    }

    //定义rpc的请求header   =>   rpc服务名称 + rpc方法 + 参数长度
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        // std::cout << "serialize rpc header error!" << std::endl;
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    //组织待发送的rpc请求的字符串   =>   头部长度  +  rpcheader(序列化的服务名称+方法名称+参数大小)  +  参数(序列化的request请求信息)
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));   //header_size
    send_rpc_str += rpc_header_str;                                //rpcheader
    send_rpc_str += args_str;                                      //args

    //打印调试信息
    std::cout << "==================请求端===================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "==========================================" << std::endl;

    //使用Tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        // std::cout << "create socket error! errno: " << error << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! error: %d", errno);
        controller->SetFailed(errtxt);
        exit(EXIT_FAILURE);
    }

    //读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    /*
    rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    */
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name;       //  /UserServiceRpc/Login
    std::string host_data = zkCli.GetData(method_path.c_str());             //127.0.0.1:8000
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //连接rpc服务节点
    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        // std::cout << "connect error! error: " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! error: %d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    //发送rpc请求
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        //std::cout << "send error! error: " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! error: %d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    //接收rpc请求响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        //std::cout << "recv error! error: " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! error: %d", errno);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    //反序列化rpc调用的响应数据
    //std::string response_str(recv_buf, 0, recv_size);   //出现bug:recv_buf中遇到\0, 那么后面的数据就存不下来了, 导致反序列化失败
    //if (!response->ParseFromString(response_str))
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        // std::cout << "parse error! recv_buf: " << recv_buf << std::endl;
        char errtxt[2048] = {0};
        sprintf(errtxt, "parse error! recv_buf: %s", recv_buf);
        controller->SetFailed(errtxt);
        close(clientfd);
        return;
    }

    close(clientfd);
}