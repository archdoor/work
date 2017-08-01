#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>

using namespace std;

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

int pack_frame(unsigned char type, unsigned char *buff)
{
    int switch_count = 3266;
    unsigned char frame_head = 0x01;
    unsigned short datalen = 0;
    unsigned short crc = 0;
    unsigned char frame_tail = 0x03;

    unsigned char *tmp = buff;

    if(type == 0x11)
    {
        cout << "0x11" << endl;
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
        memcpy(tmp, &data_buff, sizeof(data_buff));
        tmp += datalen;

        //CRC校验
        crc = get_crc(buff+1, datalen+3);
        memcpy(tmp, &crc, sizeof(crc));
        tmp += 2;

        //帧尾
        memcpy(tmp, &frame_tail, sizeof(frame_tail));
    }
    else if(type == 0x12)
    {
        cout << "0x12" << endl;
    }
    else
        cout << "save frame type" << endl;

    printf("buff:%x\n", buff);

    return 0;
}

int main()
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in seraddr={0};
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(8888);
	seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	socklen_t size = sizeof(struct sockaddr_in);

	int ret =bind(sfd, (struct sockaddr *)&seraddr, size);

	listen(sfd,10);
	
    struct sockaddr_in cliaddr = {0};
    unsigned char buff[1024] = {0};
    while(1)
    {
        memset(&cliaddr, 0, sizeof(struct sockaddr));

        int cfd = accept(sfd, (struct sockaddr *)&cliaddr, &size);
        cout << "accept a connection\n";

        //发送开关量信息数据帧
        pack_frame(0x11, buff);
        cout << "buff:" << hex << buff << endl;
        if(send(cfd, buff, 1024, 0) < 0)
        {
            cout << "send switch_value failed!\n";
        }

        //发送开关量变化信息数据帧
        //pack_frame(0x12, buff);
        //if(send(cfd, buff, 1024, 0) < 0)
        //{
        //    cout << "send switch_change_value failed!\n";
        //}
    }

	return 0;
}

