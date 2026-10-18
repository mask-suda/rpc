#include "logger.h"
#include <iostream>

// 静态成员变量的定义（初始化）
std::shared_ptr<Logger> Logger::single = nullptr;
std::mutex Logger::s_mutex;
std::once_flag Logger::s_flag;

void SafeDeletor::operator()(Logger* sf)
{
    delete sf;
}

//日志的初始化
Logger::Logger()
{
    //启动专门的写日志线程
    std::thread writeLogTask([this]{
        //简洁的while循环
        for(;;)
        {
            //获取当前的日期，然后取日志信息，写入响应的日志文件中 a+
            time_t now = time(nullptr); //now为Unix时间戳，表示从1970年1月1日到现在的秒数
            tm* nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);
            
            //打开文件，设置a追加，+可读写模式
            FILE* pf = fopen(file_name, "a+");
            if(pf == nullptr)
            {
                std::cout << "logger file: " << file_name << "open error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lckque.pop();
            
            char time_buf[28] = {0};
            sprintf(time_buf, "%d:%d:%d =>[%s] ", 
                nowtm->tm_hour, 
                nowtm->tm_min, 
                nowtm->tm_sec,
                (m_loglevel == INFO ? "info":"error")
            );
            msg.insert(0, time_buf);
            msg.append("\n");

            //将msg转化为const char*类型，写入到pf文件流中
            fputs(msg.c_str(), pf);
            fclose(pf);
        }
    });
    //设置分离线程，守护线程
    writeLogTask.detach();
}
// 设置日志级别
void Logger::SetLoglevel(LogLevel level)
{
    m_loglevel = static_cast<int>(level);
}
// 向lockqueue缓冲区中写入日志信息
void Logger::SetLogMsg(std::string msg)
{
    m_lckque.push(msg);
}

// 获取静态共享指针实例
std::shared_ptr<Logger> Logger::GetInstance()
{
    if(single != nullptr)
    {
        return single;
    }
    s_mutex.lock();
    if(single != nullptr)
    {
        s_mutex.unlock();
        return single;
    }
    single = std::shared_ptr<Logger>(new Logger, SafeDeletor());
    s_mutex.unlock();
    
    // std::call_once(s_flag, []{
	// 	single = std::shared_ptr<Logger>(new Logger, SafeDeletor());
    // });
    return single;
}