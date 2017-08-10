#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include "log.h"

//5个日志级别
static string all_log_level[5] = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};

//3个日志输出方式：控制台，日志文件，两者兼备
static string all_log_mode[3] = {"CONSOLE", "LOGFILE", "DUAL"};

static int g_log_level = 0;
static int g_log_mode = 0;
static int g_log_fd = -1;

//根据时间建立文件
int get_file_by_time(const char *path, const char *suffix, struct timeval *time = NULL)
{
    struct timeval cur_time = {0};
    if ( time == NULL )
    {
        gettimeofday(&cur_time, NULL);
        time = &cur_time;
    }

    char filename[32] = {0};
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S", localtime(&time->tv_sec));

    char pathname[64] = {0};
    strcat(pathname, path);
    strcat(pathname, filename);
    strcat(pathname, suffix);

    LogPrint("%s\n", pathname);

    int fd = open(pathname, O_CREAT | O_WRONLY | O_SYNC, 0755);
    if( fd < 0 )
    {
        LogPrint("get file %s error!\n", filename);
        return -1;
    }

    return fd;
}

//日志打印设置
int log_config(string level, string mode, const char *file_path = NULL)
{
    //设置日志文件
    if ( (mode == all_log_mode[1]) || (mode == all_log_mode[2]) )
    {
        if ( file_path == NULL )
        {
            LogPrint("log file is NULL!\n");
            return -1;
        }

        g_log_fd = get_file_by_time("../log/", ".log");
        if ( g_log_fd < 0 )
        {
            LogPrint("get_file_by_time error!\n");
            return -1;
        }
    }
    //设置日志等级
    for ( int i = 0; i < 5; ++i )
    {
        if ( level == all_log_level[i] )
        {
            g_log_level = i;
        }
    }
    //设置日志模式
    for ( int i = 0; i < 3; ++i )
    {
        if ( mode == all_log_mode[i] )
        {
            g_log_mode = i;
        }
    }

    return 0;
}

//日志打印
void log_print(int level, const char *fmt, ...)
{
    if ( g_log_level > level )
        return ;

    va_list arg_list; 
    char buff[1024] = {0};
    va_start(arg_list, fmt);
    vsnprintf(buff, sizeof(buff), fmt, arg_list);
    va_end(arg_list);

    struct timeval tv; 
    char sdate[48] = {0};
    gettimeofday(&tv, NULL);
    strftime(sdate, sizeof(sdate), "%Y-%m-%d_%H:%M:%S", localtime(&tv.tv_sec));

    char fmt_sdate[48] = {0};
    sprintf(fmt_sdate, "[%s.%.3lu]", sdate, tv.tv_usec/1000);

    //日志文件写入
    if ( g_log_mode != LOG_CONSOLE )
    {
        write(g_log_fd, fmt_sdate, strlen(fmt_sdate));
        string level_info = "[" + all_log_level[level] + "]";
        write(g_log_fd, level_info.c_str(), level_info.length());
        write(g_log_fd, buff, strlen(buff));

        //判断文件大小，过大则新建文件
        struct stat filemsg = {0};
        fstat(g_log_fd, &filemsg);
        if( filemsg.st_size > LOG_MAX_FILE_SIZE )
        {
            close(g_log_fd);
            g_log_fd = get_file_by_time("../log/", ".log");
            if ( g_log_fd < 0 )
            {
                LogWarning("get_file_by_time error!\n");
            }
        }

        if ( g_log_mode == LOG_LOGFILE )
            return ;
    }

    //日志打印
    switch( level )
    {
        case LOG_DEBUG:
            printf("%s\033[2;32;40m[DEBUG]\033[0m%s", fmt_sdate, buff);
            break;

        case LOG_INFO:
            printf("%s\033[2;34;40m[INFO]\033[0m%s", fmt_sdate, buff);
            break;

        case LOG_WARNING:
            printf("%s\033[1;33;40m[WARNING]\033[0m%s", fmt_sdate, buff);
            break;

        case LOG_ERROR:
            printf("%s\033[1;31;40m[ERROR]\033[0m%s", fmt_sdate, buff);
            break;

        case LOG_CRITICAL:
            printf("%s\033[1;31;40m[CRITICAL]\033[0m%s", fmt_sdate, buff);
            break;

        default:
            ;
    }
}

