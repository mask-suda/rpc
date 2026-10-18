#pragma once

#include <string>
#include <cstring>

//作为缓冲区
class Buffer
{
private:
    std::string net_buf;
public:
    //添加信息
    void append(const char* data, size_t len);
    //可读字节数
    size_t readableBytes() const;
    //预读指定长度（不删除已读数据）
    const std::string peek(size_t len) const;
    //返回裸指针
    const char* peekRaw() const;
    //移除指定长度数据
    void retrieve(size_t len);
    //取出并移除指定长度数据
    std::string retrieveAsString(size_t len);
};