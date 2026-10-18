#include "mprpccontroller.h"

MprpcController::MprpcController()
{
    //初始化为false，表示正常运行，未出错
    m_failed = false;
    m_errText = "";
}

void MprpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}

bool MprpcController::Failed() const
{
    return m_failed;
}

std::string MprpcController::ErrorText() const
{
    return m_errText;
}

void MprpcController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_errText = reason;
}

void MprpcController::StartCancel()
{

}
bool MprpcController::IsCanceled() const
{
    return false;
}
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback)
{

}