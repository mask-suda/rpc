#pragma once
#include "lockqueue.h"
#include <string>
#include <memory>
#include <mutex>

enum LogLevel
{
    INFO = 0,   //正常信息
    ERROR = 1   //错误信息
};

class Logger;
class SafeDeletor
{
public:
    void operator()(Logger* sf);
};

class Logger
{
private:
    int m_loglevel; //记录日志级别
    LockQueue<std::string> m_lckque; //日志缓冲队列


    //实现日志为辅助型智能指针单例模式
    static std::shared_ptr<Logger> single;
    static std::mutex s_mutex;
    static std::once_flag s_flag;

    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger &operator=(const Logger&) = delete;

    friend class SafeDeletor;
public:
    //设置日志级别
    void SetLoglevel(LogLevel level);
    //写入日志信息
    void SetLogMsg(std::string msg);

    //获取日志单例
    static std::shared_ptr<Logger> GetInstance();
};

#define LOG_INFO(logmsgformat, ...)\
do\
{\
    Logger::GetInstance()->SetLoglevel(INFO);\
    char c[1024]={0};\
    snprintf(c,1024,logmsgformat,##__VA_ARGS__);\
    Logger::GetInstance()->SetLogMsg(c);\
}while(0)

#define LOG_ERROR(logmsgformat, ...)\
do\
{\
    Logger::GetInstance()->SetLoglevel(ERROR);\
    char c[1024]={0};\
    snprintf(c,1024,logmsgformat,##__VA_ARGS__);\
    Logger::GetInstance()->SetLogMsg(c);\
}while(0)