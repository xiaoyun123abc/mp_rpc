#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"     //集成日志到框架中

/*
service_name  =>  service描述 
                        =>  service*: 记录服务对象
                        =>  method_name: method方法对象
*/
//框架开发时要考虑到：不能依赖于某个具体的业务，要可以任意的service
//这里是框架提供给外部使用的，可以发布rpc方法的函数接口
//这就是发布rpc服务的站点, 使用map保存所有Rpc服务及其对应结构体信息
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    //定义服务类型信息结构体
    ServiceInfo service_info;

    //获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();

    //获取服务的名字
    std::string service_name = pserviceDesc->name();                //UserServiceRpc

    //获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    //std::cout << "service_name:" << service_name << std::endl;      //示例输出: service_name:UserServiceRpc, 就是在user.proto中定义的rpc服务名字和方法
    LOG_INFO("service_name: %s", service_name.c_str())      //打印到日志上

    for (int i = 0; i < methodCnt; i++)
    {
        //获取了服务对象指定下标的服务方法的描述(对象描述)
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);

        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        //std::cout << "method_name:" << method_name <<std::endl;    //示例输出: method_name:Login
        LOG_INFO("method_name: %s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

//启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    //读取配置文件rpcserver的信息         获取ip+port
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    //创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    //绑定连接回调 和 消息读写回调方法    分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //设置muduo库的线程数量
    server.setThreadNum(4);   //一个是I/O线程，剩下三个是工作线程

    /*把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务*/
    // session timeout  30s   zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();           //连接zkserver 且 不断开，会主动发送ping心跳消息，维护和zkserver的连接
    // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : m_serviceMap)
    {
        // /service_name                        /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name        /UserServiceRpc/Login: 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL:表示znode是一个临时性节点(服务的方法都是临时性节点)
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);  
        }
    }

    //rpc服务端准备启动，打印信息
    std::cout << "RpcProvider start service at ip: " << ip << " port: " << port << ", waiting ......" << std::endl;

    //启动网络服务
    server.start();
    m_eventLoop.loop();      //相当于epoll_wait, 如果远程有新连接进行Tcp三次握手了，就会主动调用OnConnection
}

//新的socket连接回调方法
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        //和rpc client的链接断开了
        conn->shutdown();
    }
}


/*
在框架内部，RpcProvider和RpcConsumer之间要协商好通信用的protobuf数据类型
service_name   method_name   args   定义proto的message类型, 进行数据头的序列化和反序列化
                                    service_name method_name args_size(记录方法参数的字符串的长度，防止粘包问题)
16UserServiceLoginZhang san123456

header_size(4个字节->除了方法参数, 前面的所有数据长度) + header_str + args_str
10  "10"
10000  "10000"
std::string  insert和copy方法
直接存的话不知道怎么读取，要按二进制存在字符串的三个字节中

在 RPC 或网络通信中，二进制数据流通常按以下结构组织：
    4字节头部长度 (header_size)  +  header_size字节的头部数据  +  其他数据 (如参数)
*/
//已建立用户的读写事件回调
//muduo库如果发现有可读写的消息发生后，就会给上层主动调用此回调
//如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,       //Tcp连接
                            muduo::net::Buffer* buffer,                     //缓冲区
                            muduo::Timestamp s)                             //时间戳
{
    //网络上接收的远程rpc调用请求的字符流   Login  args
    //将其转成string字符串类型
    std::string recv_buf = buffer->retrieveAllAsString();

    //从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);   //把string recv_buf中的内容,从第0个字符开始，拷贝4个字节的内容放到header_size中

    // /*增: 网络字节序转主机字节序*/
    // header_size = ntohl(header_size);  

    //根据header_szie读取数据头的原始字符流, 反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))      //数据rpc_header_str从二进制进行反序列化，存储于rpcHeader中
    {
        //数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        //数据头反序列化失败 =>  需要记录日志
        //std::cout << "rpc_header_str: " << rpc_header_str << " parse error!" << std::endl;
        LOG_INFO("rpc_header_str: %s parse error!", rpc_header_str.c_str())
        return;
    }

    //获取rpc方法参数的字符流信息
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    //打印调试信息
    std::cout << "===================服务端==================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "==========================================" << std::endl;

    //获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        //std::cout << service_name << " is not exist!" << std::endl;
        LOG_INFO("%s is not exist!", service_name.c_str());
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        //std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        LOG_INFO("%s : %s is not exist!", service_name.c_str(), method_name.c_str());
        return;
    }

    google::protobuf::Service *service = it->second.m_service;          //获取service对象
    const google::protobuf::MethodDescriptor *method = mit->second;     //获取method对象

    //生成Rpc方法调用的请求request和响应response参数
    /*
    service->GetRequestPrototype(method): 获取指定 RPC 方法（method）的 请求消息（Request）的原型对象
    .New(): 基于原型对象 动态创建一个新的、可修改的请求消息实例。
    最终结果: request 是一个指向新创建的、空的请求消息对象的指针，后续可通过 ParseFromString 填充数据。
    */
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        //std::cout << "request parse error, content:" << args_str <<std::endl;
        LOG_INFO("request parse error, content: %s", args_str.c_str());
        return; 
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    //给下面的method方法的调用, 绑定一个Closure的回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &RpcProvider::SendRpcResponse, 
                                                                    conn, 
                                                                    response);

    //在框架上根据远端rpc请求, 调用当前rpc节点上发布的方法
    //new UserService().Login(controller, request, response, done)
    service->CallMethod(method, nullptr, request, response, done);          //UserServiceRpc回调method(Login)方法   
}

//Closure的回调操作, 用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response)
{
    std::string response_str;
    if(response->SerializeToString(&response_str))     //response进行序列化,将序列化后的结果存储于response_str中
    {
        //序列化成功后，通过网络把rpc方法执行的结果发送回rpc的调用方
        conn->send(response_str);
    }
    else
    {
        //std::cout << "Serialize response_str error!" << std::endl;
        LOG_INFO("Serialize response_str error!");
    }
    conn->shutdown();    //模拟http的短链接服务，由rpcprovider主动断开连接
}