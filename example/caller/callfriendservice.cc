#include "mprpcapplication.h"
#include "friend.pb.h"
#include <iostream>

int main(int argc, char** argv)
{
    //程序的启动后，想要使用mprpc享受rpc服务调用，一定要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);
    
    //调用远程发布的rpc方法的login
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    
    //rpc方法请求的参数
    fixbug::GetFriendListRequest getfl_req;
    getfl_req.set_userid(79);
    //rpc方法的响应
    fixbug::GetFriendListResponse getfl_rep;
    MprpcController controller;
    stub.GetFriendList(&controller, &getfl_req, &getfl_rep, nullptr);   //实际上都是RpcChannel在调用callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次rpc调用完成，所读调用的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (!getfl_rep.result().errcode())
        {
            std::cout << "rpc GetFriendList response success!" << std::endl;
            int size = getfl_rep.friends_size();
            for (int i = 0; i < size; ++i)
            {
                std::cout << "index: " << (i + 1) << "name: " << getfl_rep.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendList response error: " << getfl_rep.result().errmsg() << std::endl;
        }
    }

    return 0;
}