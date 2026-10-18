#include "mprpcprovider.h"
#include "mprpcapplication.h"
#include "mprpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

void MprpcProvider::NotifyService(google::protobuf::Service* service)
{
    ServiceInfo service_info;

    //得到服务对象的描述信息
    const google::protobuf::ServiceDescriptor* pserviceDesc = service->GetDescriptor();
    //获得服务的名字
    const std::string& service_name = pserviceDesc->name();
    //获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    //std::cout << "service_name:" << service_name << std::endl;
    LOG_INFO("service_name:%s", service_name.c_str());
    for(int i=0; i<methodCnt; ++i)
    {
        //获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        const std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});
    
        //std::cout << "method_name:" << method_name << std::endl;
        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

void MprpcProvider::Run()
{
    std::string ip = MprpcApplication::getInstance().getMpappConfig().Load("rpcserver_ip");
    uint16_t port = atoi(MprpcApplication::getInstance().getMpappConfig().Load("rpcserver_port").c_str());
    muduo::net::InetAddress address(ip,port);
    
    //创建TcpServer对象
    muduo::net::TcpServer server(&m_eventloop,address,"RpcProvider");
    //绑定连接回调和消息读写回调方法，分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&MprpcProvider::OnConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&MprpcProvider::OnMessage,this,std::placeholders::_1,
        std::placeholders::_2,std::placeholders::_3));

    //设置muduo库的线程数量
    server.setThreadNum(4);
    
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    //session timeout  30s  zkclient网络io线程  会以1/3的timeout时间发送心跳消息
    ZkClient zkCli;
    zkCli.Start();

    // service_name为永久性节点   method_name为临时性节点
    for (auto &sp : m_serviceMap)
    {
        // /service_name
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);

        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name  /userServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            //ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout << "MprpcProvider start service at ip:" << ip << " port:" << port << std::endl;
    //启动网络服务
    server.start();
    m_eventloop.loop();
}

void MprpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if(!conn->connected())//连接断开
    {
        conn->shutdown();
    }
}
/*
框架内部，provider和consumer协商通信用的protobuf数据类型
service_name method_name args   定义proto的message类型，进行数据的序列化和反序列化
为防止粘包：采用头部长度+服务名_方法名+参数信息
即header_size + header_str + args_str
示例：13UserviceLoginzhang san123456

*/
void MprpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                              muduo::net::Buffer *buffer,
                              muduo::Timestamp)
{
    // //网络中接收的远程rpc调用请求的字符流
    // std::string recv_buf = buffer->retrieveAllAsString();
    // //从字符流中读取前4个字节的内容
    // uint32_t header_size = 0;//未初始化为0，若编译器未对其进行优化，会导致其自身数据随机
    // recv_buf.copy((char*)&header_size, 4, 0);
    // //根据header_size读取数据头的原始字符流
    // std::string rpc_header_str = recv_buf.substr(4, header_size);
    // mprpc::RpcHeader rpcHeader;
    // std::string service_name;
    // std::string method_name;
    // uint32_t args_size;
    // if(rpcHeader.ParseFromString(rpc_header_str))
    // {
    //     //数据头反序列化成功
    //     service_name = rpcHeader.service_name();
    //     method_name = rpcHeader.method_name();
    //     args_size = rpcHeader.args_size();
    // }
    // else
    // {
    //     //数据头反序列化失败
    //     std::cout << "rpc_header_str:" << rpc_header_str << "parse error!" <<std::endl;
    //     return;
    // }
    // //得到参数数据信息
    // std::string args_str = recv_buf.substr(4 + header_size, args_size);

    while (buffer->readableBytes() >= 4)
    {
        std::cout << "服务端进行OnMessage,while()" << std::endl;
        /*
        uint32_t header_size = ntohl(*reinterpret_cast<const uint32_t*>(buffer->peek()));
            最终目的是从缓冲区中读取到一个4字节的长度头；
            muduo提供的buffer有读写双指针；通过peek拿到可读数据的起始地址；
            通过底层reinterpret_cast<>强制类型转换：不改变内存里的二进制位，只换一种类型去解释它；
            转换为uint32类型后，解引用得到header_size；
            最后通过ntohl将网络字节序转换为主机字节序；

            缺陷：
                若是可读数据少于4字节，解引用会越界；
                对齐问题：buffer.peek()返回的地址可能不满足uint32的对齐要求；
        */
        // 从缓冲区中预读4字节的头长度，通过memcpy安全的移动
        uint32_t net_header_size = 0;
        memcpy(&net_header_size, buffer->peek(), 4);
        uint32_t header_size = ntohl(net_header_size);

        // 半包：break结束循环，不结束函数
        if (buffer->readableBytes() < 4 + header_size)
        {
            break;
        }

        // 消费4字节长度头
        buffer->retrieve(4);
        // 读取header
        std::string rpc_header_str(buffer->peek(), header_size);
        buffer->retrieve(header_size);

        mprpc::RpcHeader rpcHeader;
        if (!rpcHeader.ParseFromString(rpc_header_str))
        {
            // 数据头反序列化失败
            std::cout << "parse rpc header error!" << std::endl;
            return;
        }
        // 反序列化成功获取数据
        std::string service_name = rpcHeader.service_name();
        std::string method_name = rpcHeader.method_name();
        uint32_t args_size = rpcHeader.args_size();

        // 半包：args_str不全
        if (buffer->readableBytes() < args_size)
        {
            break;
        }
        // 读取args_str
        std::string args_str(buffer->peek(), args_size);
        buffer->retrieve(args_size);
        // 打印调试信息
        std::cout << "+++++++++服务端打印+++++++++++" << std::endl;
        std::cout << "service_name:" << service_name << std::endl;
        std::cout << "method_name:" << method_name << std::endl;
        std::cout << "header_size:" << header_size << std::endl;
        std::cout << "rpc_header_str:" << rpc_header_str << std::endl;
        std::cout << "args_str:" << args_str << std::endl;
        std::cout << "+++++++++++++++++++++++++++++" << std::endl;

        // 获取service对象和method对象
        auto it = m_serviceMap.find(service_name);
        if (it == m_serviceMap.end())
        {
            std::cout << service_name << "is not exist!" << std::endl;
            return;
        }
        auto mit = it->second.m_methodMap.find(method_name);
        if (mit == it->second.m_methodMap.end())
        {
            std::cout << service_name << ":" << method_name << "is not exist!" << std::endl;
            return;
        }

        google::protobuf::Service *service = it->second.m_service;
        const google::protobuf::MethodDescriptor *method = mit->second;

        // 生成rpc方法调用的请求request和响应response参数
        google::protobuf::Message *request = service->GetRequestPrototype(method).New();
        if (!request->ParseFromString(args_str))
        {
            std::cout << "request parse error, content:" << args_str << std::endl;
            return;
        }
        google::protobuf::Message *response = service->GetResponsePrototype(method).New();

        // 给下面的CallMethod方法绑定一个Closure的回调函数
        google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, const muduo::net::TcpConnectionPtr &, google::protobuf::Message *>(this, &MprpcProvider::SendRpcResponse, conn, response);

        // 在框架中根据远端rpc请求，调用当前rpc节点上发布的方法
        // new UserService.Login(method,controller,request,response,done)
        service->CallMethod(method, nullptr, request, response, done);
    }
}

//Closure的回调操作，用于序列化rpc的响应和网络发送
void MprpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response)
{
    std::string response_str;
    //对response进行序列化失败
    if(!response->SerializeToString(&response_str))  
    {
        std::cout << "serialize response erroe!" << std::endl;
        //失败，服务端主动断开，防止客户端死等，并回收系统资源
        conn->shutdown();
        return;
    }
    uint32_t net_size = htonl(response_str.size());
    std::string send_str;

    //append：从指定内存地址开始，连续搬运N个字节，放到字符串末尾
    send_str.append((char*)&net_size, 4);
    send_str.append(response_str);
    //序列化成功后，通过网路吧rpc方法执行的结果发送给rpc的调用方
    conn->send(send_str);
    //模拟http的短链接服务，由rpcprovider主动断开链接
    conn->shutdown();  
}