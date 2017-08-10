#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "recever.h"
#include "system.h"
#include "log.h"


//计算CRC校验码：多项式 0x11021 X^16+X^12+X^5+1
unsigned short calc_crc(unsigned char *data, int len)
{
    unsigned short crc = 0;
    unsigned char R;
    int k, m;

    if( len <= 0 )
        return 0;

    for( int i = 0; i < len; i++ )
    {
        R = data[i];
        for( int j = 0; j < 8; j++ )
        {
            if( R > 0x7f )
                k = 1;
            else
                k = 0;

            R <<= 1;
            if( crc > 0x7fff )
                m = 1;
            else
                m = 0;

            if( k + m == 1 )
                k = 1;
            else
                k = 0;
            crc <<= 1;
            if(k == 1)
                crc ^= 0x1021;
        }
    }
    return crc;
}

DataRecever *DataRecever::m_recever = NULL;

DataRecever::DataRecever() : m_sock(0), m_file(0), m_msgid(0)
{
}

DataRecever *DataRecever::get_instance()
{
    if ( m_recever == NULL )
    {
        m_recever = new DataRecever();
    }
    return m_recever;
}

int DataRecever::init()
{
    //创建消息队列
    key_t msgkey = ftok(__FILE__, 200);
    m_msgid = msgget(msgkey, IPC_CREAT | 0644);
    if( m_msgid < 0 )
    {
        LogError("msgget fail!\n");
        return -1;
    }
    LogDebug("creat m_msgid: %d\n", m_msgid);

    //创建数据存储文件
    struct timeval cur_time = {0};
    gettimeofday(&cur_time, NULL);
    if ( create_data_file(cur_time) < 0 )
    {
        LogError("create_data_file fail!\n");
        return -1;
    }
    LogDebug("creat data file: %d\n", m_file);
    return 0;
}

//创建数据文件
int DataRecever::create_data_file(struct timeval msg_time)
{
    char filename[20] = {0};
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S", localtime(&msg_time.tv_sec));
    strcat(filename, ".data");

    m_file = open(filename, O_CREAT | O_WRONLY | O_SYNC, 0766);
    if( m_file < 0 )
    {
        LogError("open data file failed!\n");
        return -1;
    }

    return 0;
}

//数据接收线程
void *recv_thread_fn(void *args)
{
    DataRecever *recever = (DataRecever *)args;
    Mess msg;
    unsigned char *data_buff = msg.mtext;
    int recv_len = 0;

    while(1)
    {
        memset(data_buff, 0, sizeof(msg.mtext));
        recv_len = recv(recever->m_sock, data_buff, sizeof(msg.mtext) - 1, 0);
        LogDebug("recv_len: %d\n", recv_len);
        if( recv_len == 0 )
        {
            LogWarning("server outline!\n");
            if( recever->reconnect_srv() < 0 )
            {
                LogError("reconnect server fail!\n");
                pthread_exit(NULL);
            }
            else continue;
        }
        else if( recv_len < 0 ) 
        {
            LogError("recv error!\n");
            continue;
        }

        //将数据传送到消息队列
        gettimeofday(&msg.time, NULL);
        if( msgsnd(recever->m_msgid, &msg, recv_len + sizeof(long) + sizeof(struct timeval), 0) != 0 ) 
        {
            LogWarning("msgsnd error!\n");
        }
    }
    return NULL;
}

//数据去转义字符
int DataRecever::del_reverse(unsigned char *data, int datalen)
{
    unsigned char *pread = data + 1;
    unsigned char *pwrite = data + 1;
    int frame_len = datalen;
    unsigned char raw = 0x00;
    int flag = 1;

    for( int i = 1; i < datalen - 3; ++i )
    {
        flag = 1;
        unsigned short *comp = (unsigned short *)pread;
        if( (*comp ==  0x8110) || (*comp ==  0x8310) || (*comp ==  0x9010) )
        {
            frame_len--;
            pread += 2;
            switch( *comp )
            {
                case 0x8110:
                    raw = 0x01; break;
                case 0x8310:
                    raw = 0x03; break;
                case 0x9010:
                    raw = 0x10; break;
            }
        }
        else {
            raw = *pread;
            pread++;
            flag = 0;
        }

        memcpy(pwrite, &raw, 1);
        pwrite++;
    }
    if( flag == 0 )
        memcpy(pwrite, pread, 4);
    else
        memcpy(pwrite, pread, 3);

    return frame_len;
}

//数据CRC校验
int DataRecever::crc_check(unsigned char *frame, int frame_len)
{
    unsigned short crc = calc_crc(frame + 1, frame_len - 4);
    unsigned short raw_crc = 0;
    memcpy(&raw_crc, frame + frame_len - 3, sizeof(raw_crc));

    if( htons(raw_crc) != crc ) {
        return -1;
    }

    return 0;
}

//数据保存线程
void *save_thread_fn(void *args)
{
    DataRecever *recever = (DataRecever *)args;
    Mess msg;

    int msg_len = 0;
    while(1)
    {
        memset(msg.mtext, 0, sizeof(msg.mtext));
        msg_len = msgrcv(recever->m_msgid, &msg, sizeof(msg), msg.mtype, 0);
        LogDebug("msg_len: %d\n", msg_len);
        if( msg_len < 0 )
        {
            LogError("msgrcv error!\n");
            break;
        }

        int datalen = msg_len - sizeof(long) - sizeof(struct timeval);

        //数据去转义字符
        int frame_len = recever->del_reverse(msg.mtext, datalen);

        //数据校验
        if( recever->crc_check(msg.mtext, frame_len) < 0 )
        {
            LogError("CRC校验失败！\n");
            continue;
        }

        //消息时间
        char sdate[25] = {0};
        strftime(sdate, sizeof(sdate), "%Y-%m-%d_%H:%M:%S", localtime(&msg.time.tv_sec));
        strcat(sdate, "\r\n");

        //数据转化为十六进制字符
        char hex_buff[frame_len*2 + 2] = {0};
        char *tmp = hex_buff;
        for( int i = 0; i < frame_len; ++i )
        {
            sprintf(tmp, "%.2x", msg.mtext[i]);
            tmp += 2;
        }
        memcpy(tmp, "\r\n", strlen("\r\n"));

        //时间和数据写入文件
        write(recever->m_file, sdate, strlen(sdate));
        write(recever->m_file, hex_buff, frame_len*2 + 2);

        //判断文件大小，过大则新建文件
        struct stat filemsg = {0};
        fstat(recever->m_file, &filemsg);
        if( filemsg.st_size > g_data_file_size )
        {
            close(recever->m_file);
            recever->create_data_file(msg.time);
        }
    }

    return NULL;
}

//开始数据接收与存储
int DataRecever::start()
{
    if( connect_srv() < 0 ) {
        LogError("connect to server failed!\n");
        return -1;
    }

    //创建数据接收线程
    pthread_t pid_recv = 0;
    if( pthread_create(&pid_recv, NULL, recv_thread_fn, (void *)this) != 0 ) {
        LogError("create data recv thread failed!\n");
        return -1;
    }
    pthread_detach(pid_recv);

    //创建数据存储线程
    pthread_t pid_save = 0;
    if( pthread_create(&pid_save, NULL, save_thread_fn, (void *)this) != 0 ) {
        LogError("create data save thread failed!\n");
        return -1;
    }
    pthread_detach(pid_save);

    return 0;
}

//连接服务器
int DataRecever::connect_srv()
{
	m_sock = socket(AF_INET,SOCK_STREAM,0);
    if( m_sock == -1 ) 
    {
        LogDebug("create socket:%d\n", m_sock);
        return -1;
    }

	struct sockaddr_in seraddr={0};
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(g_sys_config.server_port);
	seraddr.sin_addr.s_addr = inet_addr(g_sys_config.server_ip.c_str());

	socklen_t size = sizeof(struct sockaddr_in);

    if( connect(m_sock, (struct sockaddr *)&seraddr, size) < 0 ) {
        return -1;
    }

    return 0;
}

//服务器重连
int DataRecever::reconnect_srv()
{
    int reconnect_time = 0;
    while( reconnect_time++ < g_reconnect_time )
    {
        close(m_sock);
        sleep(g_reconnect_interval);

        LogWarning("trying to reconnect... %d\n", reconnect_time);
        if( connect_srv() == 0 ) {
            return 0;
        }
    }
    return -1;
}
