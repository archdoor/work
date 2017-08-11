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

//内存搜索
int mem_search(unsigned char *membuff, int len, unsigned char byte)
{
    for( int i = 0; i < len ; ++i )
    {
        if ( membuff[i] == byte )
            return ( i + 1 );
    }
    return 0;
}

//发送消息到消息队列
int send_msgqueue(int msgid, struct Mess *msg, int msglen)
{
    msg->mtype = g_msgtype;
    gettimeofday(&msg->time, NULL);
    if( msgsnd(msgid, msg, msglen + sizeof(long) + sizeof(struct timeval), 0) != 0 ) 
    {
        LogWarning("msgsnd error!\n");
        return -1;
    }
    return 0;
}

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
        LogDebug("get new recever instance\n");
        m_recever = new DataRecever();
    }
    return m_recever;
}

int DataRecever::init()
{
    LogDebug("recever init...\n");

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
    char filename[32] = {0};
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S", localtime(&msg_time.tv_sec));

    char pathname[64] = {0};
    strcat(pathname, "../data/");
    strcat(pathname, filename);
    strcat(pathname, ".data");

    m_file = open(pathname, O_CREAT | O_WRONLY | O_SYNC, 0766);
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
    LogDebug("start data recv thread\n");

    DataRecever *recever = (DataRecever *)args;
    Mess msg = {0};
    unsigned char *data_buff = msg.mtext;
    unsigned char tmp_buff[2048] = {0};
    int frame_len = 0;

    int head_flag = 0;
    int first_flag = 1;
    while(1)
    {
        memset(tmp_buff, 0, sizeof(tmp_buff));
        int recv_len = recv(recever->m_sock, tmp_buff, sizeof(tmp_buff) - 1, 0);
        LogDebug("recv data len: %d\n", ret);
        if( recv_len <= 0 )
        {
            LogWarning("recv fail or server outline!\n");
            close(recever->m_sock);
            if( recever->reconnect_srv() < 0 )
            {
                LogError("reconnect server fail!\n");
                pthread_exit(NULL);
            }
            else continue;
        }
        //判断帧头是否正确：不正确则直接弃包
        if ( first_flag == 1 )
        {
            if ( tmp_buff[0] == 0x01 )
            {
                head_flag = 1;
            }
            else
                continue;
        }
        //检查帧尾
        int tail = mem_search(tmp_buff, recv_len, 0x03);
        if ( tail == 0 )
        {
            //继续接收
            memcpy( data_buff + frame_len, tmp_buff, recv_len );
            frame_len += recv_len;
            first_flag = 0;
            continue;
        }
        else if ( tail == recv_len )
        {
            if ( first_flag == 1 )
            {
                //直接挂到消息队列中
                send_msgqueue(recever->m_msgid, &msg, frame_len);
            }
            else
            {
                memcpy( data_buff + frame_len, tmp_buff, recv_len );
                frame_len += recv_len;
                send_msgqueue(recever->m_msgid, &msg, frame_len);
            }
        }
        else
        {
            // 消息截取
        }





        if ( frame_len + ret >= g_data_max_len )
        {
            LogWarning("recv data too long: %d\n", frame_len + ret);
        }
        else
        {
            memcpy(data_buff + frame_len, tmp_buff, ret);
            frame_len += ret;
            if ( msg.mtext[0] == 0x01 )
            {
                if ( msg.mtext[frame_len - 1] != 0x03) {
                    continue;
                }
                else
                {
                    LogDebug("recv all data len: %d\n", frame_len);
                    //将数据传送到消息队列
                    msg.mtype = g_msgtype;
                    gettimeofday(&msg.time, NULL);
                    if( msgsnd(recever->m_msgid, &msg, frame_len + sizeof(long) + sizeof(struct timeval), 0) != 0 ) 
                    {
                        LogWarning("msgsnd error!\n");
                    }
                }
            }
            else {
                LogWarning("pack head is not 0x01\n");
            }
        }
        //清空数据
        frame_len = 0;
        memset(msg.mtext, 0, sizeof(msg.mtext));
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
    LogDebug("start data save thread\n");

    DataRecever *recever = (DataRecever *)args;
    Mess msg = {0};
    msg.mtype = g_msgtype;

    int msg_len = 0;
    while(1)
    {
        memset(msg.mtext, 0, sizeof(msg.mtext));
        msg_len = msgrcv(recever->m_msgid, &msg, sizeof(msg), msg.mtype, 0);
        if( msg_len < 0 )
        {
            LogError("msgrcv error!\n");
            break;
        }

        recever->save_data_to_file( &msg, msg_len );
    }

    return NULL;
}

//开始数据接收与存储
int DataRecever::start()
{
    LogDebug("recever start running\n");

    if( connect_srv() < 0 ) {
        LogError("connect to server failed!\n");
        return -1;
    }
    LogDebug("recever connected to server\n");

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
	seraddr.sin_port = htons(g_sys_config.server_tcp_port);
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

int DataRecever::save_data_to_file(struct Mess *msg, int msglen)
{
    int datalen = msglen - sizeof(long) - sizeof(struct timeval);
    LogDebug("msgrcv raw data len: %d\n", datalen);

    //数据去转义字符
    int frame_len = del_reverse(msg->mtext, datalen);
    LogDebug("del reverse data len: %d\n", frame_len);

    //数据校验
    if( crc_check(msg->mtext, frame_len) < 0 )
    {
        LogError("CRC校验失败！\n");
        return -1;
    }

    //消息时间
    char sdate[25] = {0};
    strftime(sdate, sizeof(sdate), "%Y-%m-%d_%H:%M:%S", localtime(&msg->time.tv_sec));
    strcat(sdate, "\r\n");

    //数据转化为十六进制字符
    char hex_buff[frame_len*2 + 2] = {0};
    char *tmp = hex_buff;
    for( int i = 0; i < frame_len; ++i )
    {
        sprintf(tmp, "%.2x", msg->mtext[i]);
        tmp += 2;
    }
    memcpy(tmp, "\r\n", strlen("\r\n"));

    //时间和数据写入文件
    write(m_file, sdate, strlen(sdate));
    write(m_file, hex_buff, frame_len*2 + 2);

    //判断文件大小，过大则新建文件
    struct stat filemsg = {0};
    fstat(m_file, &filemsg);
    if( filemsg.st_size > g_data_file_size )
    {
        close(m_file);
        LogDebug("create new data file\n");
        create_data_file(msg->time);
    }
    return 0;
}

int stop()
{
    return 0;
}
