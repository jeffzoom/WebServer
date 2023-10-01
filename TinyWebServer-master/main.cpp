/*
 * @version: 
 * @Author: zsq 1363759476@qq.com
 * @Date: 2023-06-12 23:36:18
 * @LastEditors: zsq 1363759476@qq.com
 * @LastEditTime: 2023-06-13 11:15:01
 * @FilePath: /Linux_nc/WebServer_202304/TinyWebServer-master/main.cpp
 * @Descripttion: 
 */
#include "config.h"

int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "zsq";
    string passwd = "*Zz123456";
    string databasename = "yourdb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);
    

    //日志
    server.log_write();

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //触发模式
    server.trig_mode();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}