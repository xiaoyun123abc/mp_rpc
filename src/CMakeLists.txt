aux_source_directory(. SRC_LIST)    # 问题: 生成makefile文件以后, 只要cmakelists文件不做改动, cmake是不会重新生成makefile的
set(SRC_LIST 
    mprpcapplication.cc 
    mprpcconfig.cc 
    rpcheader.pb.cc 
    rpcprovider.cc 
    mprpcchannel.cc
    mprpccontroller.cc
    logger.cc
    zookeeperutil.cc)               # 将build下的makefile文件全部删除，重新编写

# muduo..是静态库, 当动态库链接静态库时会出现一些问题
# add_library(mprpc SHARED ${SRC_LIST})   
add_library(mprpc ${SRC_LIST})      # 也编成静态库

target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)
