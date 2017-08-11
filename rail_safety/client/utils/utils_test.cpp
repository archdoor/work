#ifdef UTILS_TEST

#include <iostream>
#include <unistd.h>
#include "xml_parser.h"
#include "log.h"

using namespace std;

int main()
{
    //服务器配置测试
    XmlParser config("../config/rail_safety.xml");
    if ( config.is_empty() )
    {
        cout << "load xml file failed" << endl;
        return -1;
    }

    unsigned short port = 0;
    if ( config.get_ushort("Server", "ServerTcpPort", port) != 0 )
    {
        cout << "read config fail" << endl;
        return -1;
    }
    else {
        cout << port << endl;
    }

    string ip;
    if ( config.get_string("Server", "ServerIp", ip) != 0 )
    {
        cout << "read config fail" << endl;
        return -1;
    }
    else {
        cout << ip << endl;
    }

    //日志测试
    string level;
    config.get_string("Log", "Level", level);

    string mode;
    config.get_string("Log", "Mode", mode);

    if ( log_init(level, mode, "../log/log.txt") < 0 )
    {
        cout << "log config error" << endl;
        return -1;
    }

    LogDebug("logtest:%s,%c,%d\n", "test message", 'c', 222);
    LogInfo("logtest:%s,%c,%d\n", "test message", 'c', 222);
    LogWarning("logtest:%s,%c,%d\n", "test message", 'c', 222);
    LogError("logtest:%s,%c,%d\n", "test message", 'c', 222);
    LogCritical("logtest:%s,%c,%d\n", "test message", 'c', 222);


    return 0;
}

#endif

