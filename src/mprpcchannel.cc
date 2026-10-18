#include "mprpcheader.pb.h"
#include "mprpcapplication.h"
#include "nettcpbuffer.h"
#include "zookeeperutil.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <unistd.h>

/*
header_size + service_name + method_name args_size + args
*/
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    //获取参数的序列化字符串长度
    int args_size = 0;
    std::string args_str;
    if(request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serlialize request error!");
        return;
    }

    //定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serlialize rpc header error!");
        return;
    }

    //组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    uint32_t net_header_size = htonl(header_size);
    send_rpc_str.append((char*)&net_header_size, 4);
    send_rpc_str.append(rpc_header_str);
    send_rpc_str.append(args_str);

    //打印调试信息
    std::cout << "+++++++++客户端打印+++++++++++" << std::endl;
    std::cout << "service_name:" << service_name << std::endl;
    std::cout << "method_name:" << method_name << std::endl;
    std::cout << "header_size:" << header_size << std::endl; 
    std::cout << "rpc_header_str:" << rpc_header_str << std::endl;
    std::cout << "args_str:" << args_str << std::endl;
    std::cout << "+++++++++++++++++++++++++++++" << std::endl;

    //使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)
    {
        char errtext[512] = {0};
        sprintf(errtext, "create socket error! error:%d", errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }

    // //读取配置文件rpcservice的信息
    // std::string ip = MprpcApplication::getInstance().getMpappConfig().Load("rpcserver_ip");
    // uint16_t port = atoi(MprpcApplication::getInstance().getMpappConfig().Load("rpcserver_port").c_str());
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();

    // /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    // 127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
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



    //创建服务器地址结构体，为后续connect做准备
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //尝试连接
    if(-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        char errtext[512] = {0};
        sprintf(errtext, "connect socket error! error:%d", errno);
        controller->SetFailed(errtext);
        close(clientfd);
        exit(EXIT_FAILURE);
    }
    //尝试发送
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        char errtext[512] = {0};
        sprintf(errtext, "send error! error:%d", errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }

    // //接收rpc请求的响应值
    // char recv_buf[1024] = {0};
    // int recv_size = 0;
    // if(-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    // {
    //     std::cout << "recv error! error: " << errno << std::endl;
    //     close(clientfd);
    //     return;
    // }

    // // std::string response_str(recv_buf, 0, recv_size);
    // // if(!response->ParseFromString(response_str))
    // if(!response->ParseFromArray(recv_buf, recv_size))
    // {
    //     std::cout << "parse response error! error: " << recv_buf << std::endl;
    //     close(clientfd);
    //     return;
    // }

    Buffer recv_buf;
    char temp_buf[1024] = {0};
    
    //状态变量 0等长度头，1等响应体
    int parse_state = 0;
    uint32_t resp_size = 0;

    while(true)
    {
        std::cout << "客户端处理response中,while()" << std::endl;
        int recv_size = recv(clientfd, temp_buf, 1024, 0);
        //系统调用失败
        if(-1 == recv_size)
        {
            if(errno == EINTR) continue;
            char errtext[512] = {0};
            sprintf(errtext, "recv error! error:%d", errno);
            controller->SetFailed(errtext);
            close(clientfd);
            return;
        }
        //对端正常关闭链接
        if(0 == recv_size)
        {
            std::cout << "server closed connection!" << std::endl;
            close(clientfd);
            return;
        }
        //追加到累计缓冲区
        recv_buf.append(temp_buf, recv_size);
        //状态机循环解析
        bool k_parsing = true;
        while (k_parsing)
        {
            k_parsing = false;
            //等待4字节头
            if(parse_state == 0)
            {
                if(recv_buf.readableBytes() >= 4)
                {
                    uint32_t net_size;
                    memcpy(&net_size, recv_buf.peekRaw(), 4);
                    resp_size = ntohl(net_size);
                    recv_buf.retrieve(4);
                    parse_state = 1;
                    k_parsing = true;
                }
            }
            //等待完整的响应体
            if(parse_state == 1)
            {
                if(recv_buf.readableBytes() >= resp_size)
                {
                    std::string resp_str = recv_buf.peek(resp_size);
                    recv_buf.retrieve(resp_size);
                    if(!response->ParseFromString(resp_str))
                    {
                        char errtext[512] = {0};
                        sprintf(errtext, "parse response error! resp_str:%s", resp_str.c_str());
                        controller->SetFailed(errtext);
                        close(clientfd);
                        return;
                    }
                    std::cout << "客户端处理response完毕,while()结束" << std::endl;
                    parse_state = 0;
                    k_parsing = true;
                }
            }
        }
    }
    close(clientfd);
}