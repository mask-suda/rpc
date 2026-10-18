#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

//全局watcher观察期 zkerver给zkclient的通知
void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx)
{
    if(type == ZOO_SESSION_EVENT) //回调的消息类型是和会话相关的消息类型
    {
        if(state == ZOO_CONNECTED_STATE) //zkclient和zkserver连接成功
        {
            sem_t* sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient():m_zhandle(nullptr){}

ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle);
        m_zhandle = nullptr;
    }
}

// zkclient启动连接zkerver
void ZkClient::Start()
{
    std::string host = MprpcApplication::getInstance().getMpappConfig().Load("zkserver_ip");
    std::string port = MprpcApplication::getInstance().getMpappConfig().Load("zkserver_port");
    std::string connstr = host + ":" + port;
    
    /*
    zookeeper_mt:多线程版本
    zookeeper的API客户端程序提供了三个线程：
    API调用线程
    网络IO线程 pthread_create poll
    wather回调线程
    */
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if(m_zhandle == nullptr)
    {
        std::cout << "zookeeper_init error!" <<std::endl;
        exit(EXIT_FAILURE);
    }
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    std::cout << "zookeeper_intit success!" << std::endl;
}

// 在zkserver中根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    //先判断path表示的znode节点是否存在，如果存在就不需要重复创建
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (ZNONODE == flag)
    {
        flag = zoo_create(m_zhandle, path, data, datalen,
                &ZOO_OPEN_ACL_UNSAFE, 
                state,
                path_buffer, bufferlen);
        if (flag == ZOK)
        {
            std::cout << "znode create success... path:" << path << std::endl;
        }
        else
        {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error... path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// 根据参数指定znode的节点路径，或者znode节点的值
std::string ZkClient::GetData(const char* path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "get znode error... path:" << path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}