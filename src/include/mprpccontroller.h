#pragma once

#include <google/protobuf/service.h>


class MprpcController : public google::protobuf::RpcController
{
private:
    bool m_failed;  //rpc方法执行中的状态
    std::string m_errText;  //rpc方法执行中的错误信息
public:
    MprpcController();
    //重置为调用成功的状态
    void Reset();
    //返回调用状态
    bool Failed() const;
    //返回调用信息
    std::string ErrorText() const;
    //写入发生错误信息
    void SetFailed(const std::string& reason);

    //尚未实现
    void StartCancel();
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);
};