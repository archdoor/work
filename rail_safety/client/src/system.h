#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "config_param.h"

#define  VERSION      "V1.0(B1610607)"
#define  PROC_NAME    "rail_safety"
#define  VER_FILE     "/var/run/SCP/" PROC_NAME ".txt"

extern config_param_t    g_sys_config;

int SystemInit(int argc, char **argv);
void SystemDeinit();


#endif

