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
    unsigned short server_port;
    std::string server_ip;

}config_param_t;


#endif
