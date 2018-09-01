//
// Created by root on 27/08/18.
//

#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <opencv2/core.hpp>
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
    //const std::string server_addr= fs["IP_addr_listen"];
    const int timeout_max = fs["timeout_max"];


    //========== create socket ===========//
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==sockfd){
        puts("Failed to create socket");
        return false;
    }

    // ========= set address and port =========//
    struct sockaddr_in addr;
    socklen_t          addr_len=sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;       // Use IPV4
    addr.sin_port   = htons(port_out);    //
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //const char* serveraddr = server_addr.c_str();
    //addr.sin_addr.s_addr = inet_addr(serveraddr); // IP address of server

    // ========= set listen interval =========== //
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = receiver_interval*1000;  // millisecond to usecond
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    //========== Bind ports. ===========//
    // The receiver must bind port, the ports no. of server and listener should be the same.
    if (bind(sockfd, (struct sockaddr*)&addr, addr_len) == -1){
        printf("Failed to bind socket on port %d\n", port_out);
        close(sockfd);
        return false;
    }

    char buffer[buf_size];
    memset(buffer, 0, buf_size);
    int counter = 0, timeout_cnt = 0;
    //=========== listen to addr and port =============//
    while(1){
        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));

        int sz = recvfrom(sockfd, buffer, buf_size, 0, (sockaddr*)&src, &src_len);
        if (sz > 0){
            buffer[sz] = 0;
            printf("Get Message %d: %s\n", counter++, buffer);
            timeout_cnt = 0;
        }
        else{
            timeout_cnt++;
            printf("No data: %d\n", timeout_cnt);
            if (timeout_cnt > timeout_max)
                break;
        }
    }

    printf("No data is received from port:%d \n", port_out);
    close(sockfd);
    return 0;
}