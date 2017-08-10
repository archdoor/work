#ifndef __RECEVER_H__
#define __RECEVER_H__


//数据包最大长度
const int g_packet_len = 1200;
//数据文件最大长度
const int g_data_file_size = 20*1024*1024;
//服务器重连次数
const int g_reconnect_time = 5;
//服务器重连时间间隔
const int g_reconnect_interval = 5;

//消息队列中的消息结构
struct Mess
{
    long mtype = 100;
    struct timeval time;
    unsigned char mtext[g_packet_len];
}__attribute__ ((packed));

//服务器数据接收，并存储到文件中
class DataRecever
{
public:
    DataRecever();
    static DataRecever *get_instance();

    int init();
    int create_data_file(struct timeval msg_time);
    int start();
    int connect_srv();
    int reconnect_srv();
    int del_reverse(unsigned char *data, int datalen);
    int crc_check(unsigned char *frame, int frame_len);

    int m_sock;
    int m_file;
    int m_msgid;

private:
    static DataRecever *m_recever;
};




#endif
