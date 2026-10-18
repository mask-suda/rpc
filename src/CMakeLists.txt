#aux_source_directory(. SRC_LIST)
# 定义 THREADED 宏，启用多线程同步 API
add_definitions(-DTHREADED)
set(SRC_LIST
    mprpcapplication.cc
    mprpcconfig.cc
    mprpcheader.pb.cc
    mprpcprovider.cc
    mprpcchannel.cc
    nettcpbuffer.cc
    mprpccontroller.cc
    logger.cc
    zookeeperutil.cc
)
add_library(mprpc ${SRC_LIST})
target_link_libraries(
    mprpc 
    muduo_net 
    muduo_base 
    pthread 
    /usr/local/zookeeper/lib/libzookeeper_mt.so
)