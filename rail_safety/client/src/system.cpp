#include <iostream>
#include "system.h"
#include "xml_parser.h"
#include "recever.h"
#include "log.h"

config_param_t    g_sys_config;
const char *config_file = "../config/rail_safety.xml";

//获取系统配置
int load_sys_config()
{
    XmlParser config(config_file);
    if ( config.is_empty() ) 
    {
        LogError("load xml file failed\n");
        return -1;
    }

    //服务器IP
    if ( config.get_string("Server", "ServerIp", g_sys_config.server_ip) != 0 )
    {
        LogError("read config fail\n");
        return -1;
    }

    //服务器端口
    if ( config.get_ushort("Server", "ServerTcpPort", g_sys_config.server_port) != 0 )
    {
        LogError("read config fail\n");
        return -1;
    }

    //日志等级
    if ( config.get_string("Log", "Level", g_sys_config.log_level) != 0 )
    {
        LogError("read config fail\n");
        return -1;
    }

    //日志输出模式
    if ( config.get_string("Log", "Mode", g_sys_config.log_mode) != 0 )
    {
        LogError("read config fail\n");
        return -1;
    }

    return 0;
}

int SystemInit(int argc, char** argv)
{
    LogDebug("start system init...\n");

    //系统配置
    if ( load_sys_config() != 0 )
    {
        LogError("load_sys_config fail!\n");
        return -1;
    }

    //日志设置
    if ( log_config( g_sys_config.log_level, g_sys_config.log_mode, "../log" ) < 0 )
    {
        LogError("log_config error!\n");
        return -1;
    }

    //服务端数据接收与保存线程
    DataRecever *recever = DataRecever::get_instance();
    if ( recever->init() < 0 )
    {
        LogError("recever init fail!\n");
        return -1;
    }
    if( recever->start() < 0 )
    {
        LogError("recever start fail!\n");
        return -1;
    }

	return 0;
}

void SystemDeinit()
{
}
