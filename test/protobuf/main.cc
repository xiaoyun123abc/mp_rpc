#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;

int main()
{
    //message中存在其它message时的赋值方式
    LoginResponse rsp;
    ResultCode *rc1 = rsp.mutable_result();
    rc1->set_errcode(0);
    rc1->set_errmsg("登录处理失败");

    //列表赋值方式
    GetFriendListsResponse res;
    ResultCode *rc2 = res.mutable_result();
    rc2->set_errcode(0);

    User *user1 = res.add_friend_list();
    user1->set_name("zhao");
    user1->set_age(25);
    user1->set_sex(User::MAN);

    User *user2 = res.add_friend_list();
    user2->set_name("xue");
    user2->set_age(25);
    user2->set_sex(User::WOMAN);

    std::cout << res.friend_list_size() << std::endl;

    return 0;
}

int main1()
{
    //封装了login请求对象的数据
    LoginRequest req;
    req.set_name("张三");
    req.set_pwd("123456");

    //对象数据序列化  =>  char*  =>  可以通过网络发送
    std::string send_str;
    if (req.SerializeToString(&send_str))
    {
        std::cout << send_str.c_str() << std::endl;
    }

    //从send_str反序列化一个login请求对象
    LoginRequest reqB;
    if (reqB.LoginRequest::ParseFromString(send_str))
    {
        std::cout << reqB.name() << std::endl;
        std::cout << reqB.pwd() << std::endl;
    }

    return 0;
}