#include "mprpcapplication.h"
#include "mprpcchannel.h"
#include "user.pb.h"
#include <iostream>

int main(int argc, char** argv)
{
    //程序的启动后，想要使用mprpc享受rpc服务调用，一定要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);
    
    //调用远程发布的rpc方法的login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    
    //rpc方法请求的参数
    fixbug::LoginRequest login_req;
    login_req.set_name("AeterLyn");
    login_req.set_pwd("123456");
    //rpc方法的响应
    fixbug::LoginResponse login_rep;
    stub.Login(nullptr, &login_req, &login_rep, nullptr);   //实际上都是RpcChannel在调用callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    
    //一次rpc调用完成，所读调用的结果
    if(!login_rep.result().errcode())
    {
        std::cout << "rpc login response success: " << login_rep.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error: " << login_rep.result().errmsg() << std::endl;
    }

    //调用远程发布的rpc方法的Register
    fixbug::RegisterRequest register_req;
    register_req.set_id(79);
    register_req.set_name("baikai");
    register_req.set_pwd("654321");

    //以同步的方式调用rpc方法请求，等待返回结果
    fixbug::RegisterResponse register_rep;
    stub.Register(nullptr, &register_req, &register_rep, nullptr);
    //一次rpc调用完成，所读调用的结果
    if(!register_rep.result().errcode())
    {
        std::cout << "rpc register response success: " << register_rep.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error: " << register_rep.result().errmsg() << std::endl;
    }
    return 0;
}