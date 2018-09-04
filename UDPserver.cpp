#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "udpSocketSimple.h"

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
    const int send_inverval = fs["Send_inverval"];

    //int port_in  = 8888;
    //int port_out = 8889;
    // create socket
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==sockfd){
        puts("Failed to create socket");
        exit(-1);
    }

    // (IN) Set IP address and port
    struct sockaddr_in addr;
    socklen_t          addr_len=sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;       // Use IPV4
    addr.sin_port   = htons(port_in);    //
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //addr.sin_addr.s_addr = inet_addr("192.168.0.1");

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
        // (OUT) Set IP address and port
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port_out);
        //addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_addr.s_addr = inet_addr("146.169.203.165"); // Set
        //std::string send_str = "Hello world " + std::to_string(counter);
        std::string send_str = std::to_string(1);

        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));

        sendto(sockfd, send_str.c_str(), send_str.length(), 0, (sockaddr*)&addr, addr_len);
        printf("Sended %d\n", ++counter);
        usleep(send_inverval*1e3);
    }

    close(sockfd);
    return 0;
}