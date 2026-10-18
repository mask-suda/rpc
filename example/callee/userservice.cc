#include <iostream>
#include <string>
#include <thread>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"

class UserService : public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name, std::string pwd){
        std::cout << "doing local service : login" << std::endl;
        std::cout << "name: " << name << std::endl << "pwd: " << pwd <<std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service : register" << std::endl;
        std::cout << "uint32_id: "<< id << "name: " << name << std::endl << "pwd: " << pwd <<std::endl;
        return true;
    }

    //重写基类UserServiceRpc的虚函数，这个方法是框架直接调用的
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done){
        //执行本地服务
        std::string name = request->name();
        std::string pwd = request->pwd();
        bool login_result = Login(name,pwd);
        //写入返回信息
        fixbug::ResultCode* rc = response->mutable_result();
        rc->set_errcode(0);
        rc->set_errmsg("");
        response->set_success(login_result);
        //执行回调操作
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }
                       
};


int main(int argc,char** argv)
{
  
    //初始化网络框架
    std::cout<<"测试 初始化网络框架"<<std::endl;
    MprpcApplication::Init(argc,argv);

    //provider是一个rpc网络服务对象，将UserService对象发布到rpc节点上
    std::cout<<"测试 将UserService对象发布到rpc节点上"<<std::endl;
    MprpcProvider provider;
    provider.NotifyService(new UserService());

    //启动rpc服务节点，Run进去之后，进程进入阻塞状态，等待远程rpc调用请求
    provider.Run();
    return 0;
}