#include <iostream>
#include <unistd.h>
#include "mprpcapplication.h"

MprpcConfig MprpcApplication::m_config;   // 静态成员的定义

void showArgsHelp()
{
    std::cout << "format: command -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc,char** argv)
{
    if(argc < 2){
        showArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while((c = getopt(argc,argv,"i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            showArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            showArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    //开始加载配置文件
    m_config.LoadConfigFile(config_file.c_str());
    
    std::cout << "rpcserver_ip:" << m_config.Load("rpcserver_ip") << std::endl;
    std::cout << "rpcserver_port:" << m_config.Load("rpcserver_port") << std::endl;
    std::cout << "zkserver_ip:" << m_config.Load("zkserver_ip") << std::endl;
    std::cout << "zkserver_port:" << m_config.Load("zkserver_port") << std::endl;
}

MprpcApplication& MprpcApplication::getInstance()
{
    static MprpcApplication instance;
    return instance;
}

MprpcConfig& MprpcApplication::getMpappConfig()
{
    return m_config;
}