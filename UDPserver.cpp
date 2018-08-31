#include<sys/select.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>

int main(){
    //========= Load parameters ==========//
    const std::string &strSetting = "../settings.yaml";
    cv::FileStorage fs(strSetting, cv::FileStorage::READ);
    if(!fs.isOpened())
    {
        std::cerr << "Failed to open setting file! Exit!" << std::endl;
        exit(-1);
    }

    const int port_in = fs["Port_in"];
    const int port_out = fs["Port_out"];
    const int receiver_interval = fs["Receiver_interval"];
    const int buf_size = fs["Buf_size"];
    const std::string server_addr= fs["IP_addr_listen"];
    const int timeout_max = fs["timeout_max"];

    //int port_in  = 8888;
    //int port_out = 8889;
    int sockfd;

    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==sockfd){
        puts("Failed to create socket");
        return false;
    }

    // 设置地址与端口
    struct sockaddr_in addr;
    socklen_t          addr_len=sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;       // Use IPV4
    addr.sin_port   = htons(port_in);    //
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Time out
    /*
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 200000;  // 200 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    */
    // ============ bind port, for listening =========== //
    if (bind(sockfd, (struct sockaddr*)&addr, addr_len) == -1){
        printf("Failed to bind socket on port %d\n", port_in);
        close(sockfd);
        return false;
    }

    int counter = 0;
    while(1){
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port_out);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        std::string send_str = "Hello world " + std::to_string(counter);

        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));

        sendto(sockfd, send_str.c_str(), send_str.length(), 0, (sockaddr*)&addr, addr_len);
        printf("Sended %d\n", ++counter);
        sleep(1);
    }

    close(sockfd);
    return 0;
}