#include <iostream>
#include <unordered_map>
#include "mprpcconfig.h"

// 对配置文件信息进行处理
std::string trim(const std::string& s)//去掉key/value前后的空格
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if(start == std::string::npos) return "";

    //find_first_not_of()返回下标
    size_t end = s.find_last_not_of(" \t\n\r");

    //substr(起始位置，需要截取的长度)
    return s.substr(start,end-start+1);
}

void process(const char *buf, std::unordered_map<std::string,std::string>& m_configMap)
{
    /*
        对配置文件内部信息进行处理
        1.去掉空行和注释行
        2.提取=号两边内容为key和value
        3.删除key/value两端空白
        4.将数据存入map
    */
    //跳过空行和注释
    if(buf[0] == '\0' || buf[0] == '#') return;
    //提取key和value
    std::string str_buf(buf);

    size_t eq_pos = str_buf.find('=');
    if(eq_pos == std::string::npos) return;

    std::string key = trim(str_buf.substr(0,eq_pos));
    std::string value = trim(str_buf.substr(eq_pos+1));

    if(!key.empty())
    {
        m_configMap.insert({key,value});
    }
}

// 解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE* pf = fopen(config_file,"r");
    if(pf == nullptr)
    {
        std::cout << config_file << "is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    char buf[512] = {0};
    while(fgets(buf,sizeof(buf),pf) != nullptr)
    {
        process(buf,m_configMap);
    }

    fclose(pf);
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if(it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}