#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

const int server_tcp_port = 8888;
const int packet_len = 1520;
const char server_ip[] = "127.0.0.1";

int main()
{
	int cfd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in seraddr={0};
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(server_tcp_port);
	seraddr.sin_addr.s_addr = inet_addr(server_ip);

	socklen_t size = sizeof(struct sockaddr_in);

	int ret = connect(cfd, (struct sockaddr *)&seraddr, size);

    char buff[packet_len] = {0};
    int len = 0;
    if( (len = recv(cfd, buff, packet_len-1, 0)) < 0)
    {
        cout << "send failed!\n";
    }
    cout << buff << endl;
    printf("%d\n", len);
    printf("%x\n", buff);

	close(cfd);

    return 0;
}

