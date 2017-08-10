#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include "log.h" 

using namespace std;

int intervel_time_0 = 10;
int intervel_time_1 = 1;

int g_frame_len = 1520;

struct Param
{
    int fd;
    struct sockaddr_in addr; 
};

unsigned short get_crc(unsigned char *data, int len)
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

int reverse_frame(unsigned char *frame, unsigned short frame_len)
{
    unsigned char buff[g_frame_len] = {0};
    unsigned char *tmp = buff;
    int lenth = frame_len;

    memcpy(tmp++, &frame[0], 1);
    for( int i = 1; i < frame_len - 1; ++i )
    {
        if( frame[i] == 0x01 )
        {
            unsigned short reverse = 0x8110;
            memcpy(tmp, &reverse, 2);
            tmp += 2;
            lenth += 1;
        }
        else if( frame[i] == 0x03 )
        {
            unsigned short reverse = 0x8310;
            memcpy(tmp, &reverse, 2);
            tmp += 2;
            lenth += 1;
        }
        else if( frame[i] == 0x10 )
        {
            unsigned short reverse = 0x9010;
            memcpy(tmp, &reverse, 2);
            tmp += 2;
            lenth += 1;
        }
        else
        {
            memcpy(tmp, &frame[i], 1);
            tmp += 1;
        }
    }
    memcpy(tmp, &frame[frame_len - 1], 1);
    memcpy(frame, buff, lenth);

    return lenth;
}

int pack_frame(unsigned char type, unsigned char *buff)
{
    int switch_count = 3266;
    unsigned short switch_change_count = 0x9f;
    unsigned char frame_head = 0x01;
    unsigned short datalen = 0;
    unsigned short crc = 0;
    unsigned char frame_tail = 0x03;

    unsigned char *tmp = buff;

    if(type == 0x11)
    {
        //帧头
        memcpy(tmp, &frame_head, sizeof(frame_head));
        tmp += 1;

        //帧类型
        memcpy(tmp, &type, sizeof(type));
        tmp += 1;

        //数据段长度
        datalen = switch_count % 8 > 0 ? switch_count/8 + 1 : switch_count/8;
        memcpy(tmp, &datalen, sizeof(datalen));
        tmp += 2;

        //数据段
        unsigned char data_buff[datalen] = {0};
        memset(data_buff, 0xff, datalen);
        memcpy(tmp, &data_buff, sizeof(data_buff));
        tmp += datalen;

        //CRC校验
        crc = get_crc(buff+1, datalen+3);
        unsigned short r_crc = htons(crc);
        memcpy(tmp, &r_crc, sizeof(r_crc));
        tmp += 2;

        //帧尾
        memcpy(tmp, &frame_tail, sizeof(frame_tail));
    }
    else if(type == 0x12)
    {
        //帧头
        memcpy(tmp, &frame_head, sizeof(frame_head));
        tmp += 1;

        //帧类型
        memcpy(tmp, &type, sizeof(type));
        tmp += 1;

        //数据段长度
        datalen = switch_change_count * 2 + 2;
        memcpy(tmp, &datalen, sizeof(datalen));
        tmp += 2;

        //数据段
        unsigned char data_buff[datalen] = {0};
        memcpy(tmp, &switch_change_count, sizeof(switch_change_count));
        tmp += 2;
        unsigned short change_velue = 0xcaff;
        for( int i = 0; i < switch_change_count; ++i )
        {
            memcpy(tmp, &change_velue, sizeof(change_velue));
            tmp += 2;
        }

        //CRC校验
        crc = get_crc(buff+1, datalen+3);
        unsigned short r_crc = htons(crc);
        memcpy(tmp, &r_crc, sizeof(r_crc));
        tmp += 2;

        //帧尾
        memcpy(tmp, &frame_tail, sizeof(frame_tail));
    }
    else
        cout << "save frame type" << endl;

    LogPrint("转义前：\n");
    LogMemPrint(buff, datalen + 7);

    int frame_len = reverse_frame(buff, datalen + 7);

    LogPrint("转义后：\n");
    LogMemPrint(buff, frame_len);

    return frame_len;
}

void *send_data_thread_fn(void *args)
{
    struct Param *arguments = (struct Param *)args;
    int cfd = arguments->fd;
    unsigned char buff[1024] = {0};

    while(1)
    {
        memset(buff, 0, sizeof(buff));

        //发送开关量信息数据帧
        int frame_len = pack_frame(0x11, buff);
        int ret = send(cfd, buff, frame_len, 0);
        if( ret < 0)
        {
            cout << "connection interrupt: " << inet_ntoa(arguments->addr.sin_addr) << endl;
            free(arguments);
            pthread_exit(NULL);
        }

        sleep(intervel_time_1);

        //发送开关量变化信息数据帧
        frame_len = pack_frame(0x12, buff);
        ret = send(cfd, buff, frame_len, 0);
        if(ret < 0)
        {
            cout << "connection interrupt: " << inet_ntoa(arguments->addr.sin_addr) << endl;
            free(arguments);
            pthread_exit(NULL);
        }

        sleep(intervel_time_0);
    }
}


int main()
{
	signal(SIGPIPE,SIG_IGN);

	int sfd = socket(AF_INET, SOCK_STREAM, 0);

	int on = 1;	
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	struct sockaddr_in seraddr={0};
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(8888);
	seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	socklen_t size = sizeof(struct sockaddr_in);

    if( bind(sfd, (struct sockaddr *)&seraddr, size) < 0 )
    {
        cout << "bind fail" << endl;
        return -1;
    }

	listen(sfd,10);
	
    while(1)
    {
        struct Param *arguments = (struct Param *)malloc(sizeof(struct Param));
        memset(arguments, 0, sizeof(struct Param));

        arguments->fd = accept(sfd, (struct sockaddr *)&arguments->addr, &size);
        cout << "accept a connection from: " << inet_ntoa(arguments->addr.sin_addr) << endl;

        pthread_t pid = 0;
        if( pthread_create(&pid, 0, send_data_thread_fn, (void *)arguments) < 0 ) {
            cout << "create send data thread fail!" << endl;
        }
        pthread_detach(pid);
    }

	return 0;
}

