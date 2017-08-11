#ifndef _CONFIG_PARAM_H
#define _CONFIG_PARAM_H

#include <iostream>

using std::string;


typedef struct ConfigParam
{
    //Log
    string log_level;
    string log_mode;

    // Server
    string server_ip;
    unsigned short server_tcp_port;

}config_param_t;


#endif
