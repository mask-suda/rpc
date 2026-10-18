#include "nettcpbuffer.h"

// 添加信息
void Buffer::append(const char *data, size_t len)
{
    net_buf.append(data, len);
}

// 可读字节数
size_t Buffer::readableBytes() const
{
    return net_buf.size();
}

// 预读指定长度（不删除已读数据）
const std::string Buffer::peek(size_t len) const
{
    if (len > net_buf.size())
        len = net_buf.size();
    return net_buf.substr(0, len);
}
//返回裸指针
const char* Buffer::peekRaw() const
{
    return net_buf.data();
}
// 移除指定长度数据
void Buffer::retrieve(size_t len)
{
    if (len >= net_buf.size())
    {
        net_buf.clear();
    }
    else
    {
        net_buf.erase(0, len);
    }
}
// 取出并移除指定长度数据
std::string Buffer::retrieveAsString(size_t len)
{
    std::string result = net_buf.substr(0, len);
    retrieve(len);
    return result;
}