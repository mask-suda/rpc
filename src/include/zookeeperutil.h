#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

class ZkClient
{
private:
    //zk的客户端句柄
    zhandle_t* m_zhandle;
public:
    ZkClient();
    ~ZkClient();

    //zkclient启动连接zkerver
    void Start();

    //在zkserver中根据指定的path创建znode节点
    void Create(const char* path, const char* data, int datalen, int state = 0);

    //根据参数指定znode的节点路径，或者znode节点的值
    std::string GetData(const char* path);
};