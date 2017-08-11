#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "log.h"

//日志文件互斥锁
static pthread_mutex_t log_mutex;

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

    int fd = open(pathname, O_CREAT | O_WRONLY | O_SYNC, 0755);
    if( fd < 0 )
    {
        LogPrint("get file %s error!\n", filename);
        return -1;
    }

    return fd;
}

//日志打印设置
int log_init(string level, string mode, const char *file_path = NULL)
{
    //设置日志文件
    if ( (mode == all_log_mode[1]) || (mode == all_log_mode[2]) )
    {
        if ( file_path == NULL )
        {
            LogError("log file path can't be NULL!\n");
            return -1;
        }

        g_log_fd = get_file_by_time("../log/", ".log");
        if ( g_log_fd < 0 )
        {
            LogError("get_file_by_time error!\n");
            return -1;
        }
        LogDebug("get log file fd: %d\n", g_log_fd);

        //初始化日志文件互斥锁
        if (pthread_mutex_init(&log_mutex, NULL))
        {   
            LogError("failed to init log mutex!\n");
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
    LogDebug("set log level: %d\n", g_log_level);
    //设置日志模式
    for ( int i = 0; i < 3; ++i )
    {
        if ( mode == all_log_mode[i] )
        {
            g_log_mode = i;
        }
    }
    LogDebug("set log mode: %d\n", g_log_mode);

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
        //获取互斥量
        pthread_mutex_lock( &log_mutex );

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
        //释放互斥量
        pthread_mutex_unlock( &log_mutex );

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

