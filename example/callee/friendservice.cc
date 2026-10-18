#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"
#include "logger.h"

class FriendService : public fixbug::FriendServiceRpc
{
public:

    std::vector<std::string> GetFriendList(uint32_t id)
    {
        std::cout << "doing GetFriendList service!" << std::endl;
        std::vector<std::string> vec;
        vec.push_back("wang yang");
        vec.push_back("Ater Lyn");
        return vec;
    }

    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendListRequest* request,
                       ::fixbug::GetFriendListResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t user_id = request->userid();
        std::vector<std::string> friendList = GetFriendList(user_id);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for(std::string& name : friendList)
        {
            response->add_friends(name);
        }
        done->Run();
    }
};

int main(int argc,char** argv)
{
    LOG_INFO("first log message!");
    LOG_ERROR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);

    //初始化网络框架
    std::cout<<"测试 初始化网络框架"<<std::endl;
    MprpcApplication::Init(argc,argv);

    //provider是一个rpc网络服务对象，将UserService对象发布到rpc节点上
    std::cout<<"测试 将UserService对象发布到rpc节点上"<<std::endl;
    MprpcProvider provider;
    provider.NotifyService(new FriendService());

    //启动rpc服务节点，Run进去之后，进程进入阻塞状态，等待远程rpc调用请求
    provider.Run();
    return 0;
}