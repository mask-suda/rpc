#pragma once
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"
//mprpc的框架类
class MprpcApplication
{
private:
    static MprpcConfig m_config;

    MprpcApplication(){};
    MprpcApplication(const MprpcApplication&) = delete;                 //拷贝构造
    MprpcApplication& operator = (const MprpcApplication&) = delete;    //拷贝赋值
    MprpcApplication(MprpcApplication&&) = delete;                      //移动构造
    MprpcApplication& operator = (MprpcApplication&&) = delete;         //移动赋值

public:
    static void Init(int argc,char** argv);
    static MprpcApplication& getInstance();
    static MprpcConfig& getMpappConfig();
};