//
// Created by root on 31/08/18.
//
#include "udpSocketSimple.h"
#include <string>
#include <thread>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

namespace skaegy
{

udpSocket::udpSocket(const string strSettingFile)
{
    cv::FileStorage fs(strSettingFile, cv::FileStorage::READ);
    if(!fs.isOpened())
    {
        std::cerr << "Failed to open setting file! Exit!" << std::endl;
        exit(-1);
    }

    mPortIn = fs["Port_in"];
    mPortOut = fs["Port_out"];
    mSenderInterval = fs["Send_inverval"];
    mReceiverInterval = fs["Receiver_interval"];
    const string server_addr = fs["IP_addr_listen"];
    mIPListenToServer = server_addr.c_str();
    mTimeOutMax = fs["timeout_max"];

    CreateServerSocket(mPortIn);
    CreateClientSocket(mPortOut, mReceiverInterval);

}

void udpSocket::RunServer(){
    int counter = 0;

    while(1){
        socklen_t addr_len=sizeof(addrServer);
        addrServer.sin_family = AF_INET;
        addrServer.sin_port   = htons(mPortOut);
        addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
        std::string send_str = "Hello world " + std::to_string(counter);

        sendto(mSockfdServer, send_str.c_str(), send_str.length(), 0, (sockaddr*)&addrServer, addr_len);
        printf("Server: Sended %d\n", ++counter);
        usleep(mSenderInterval*1e3);

        if (mCloseServer)
            break;
    }
    puts("Server is closed!");
    close(mSockfdServer);
}

void udpSocket::Run() {
    int counter = 0, timeout_cnt = 0;

    while(1){
        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));

        int sz = recvfrom(mSockfdClient, mbuffer, mBuffersize, 0, (sockaddr*)&src, &src_len);
        if (sz > 0){
            mbuffer[sz] = 0;
            printf("Get Message %d: %s\n", counter++, mbuffer);
            timeout_cnt = 0;
        }
        else{
            timeout_cnt++;
            printf("No data: %d\n", timeout_cnt);
            if (timeout_cnt > mTimeOutMax)
                break;
        }

        if (mCloseClient){
            close(mSockfdClient);
            break;
        }
    }
    printf("No data is received from: %s:%d \n", mIPListenToServer, mPortOut);
    close(mSockfdClient);
}

bool udpSocket::CreateServerSocket(int port_in){
    // create socket
    mSockfdServer = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSockfdServer == -1){
        puts("Server: Failed to create server socket");
        exit(-1);
    }
    // Set IP address and port
    socklen_t  addr_len=sizeof(addrServer);

    memset(&addrServer, 0, sizeof(addrServer));
    addrServer.sin_family = AF_INET;       // Use IPV4
    addrServer.sin_port   = htons(port_in);    //
    addrServer.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(mSockfdServer, (struct sockaddr*)&addrServer, addr_len) == -1){
        printf("Server: Failed to bind socket on port %d\n", port_in);
        close(mSockfdServer);
        return false;
    }

    return 0;
}

bool udpSocket::CreateClientSocket(int port_out, int receiver_interval){
    // create socket
    mSockfdClient = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==mSockfdClient){
        puts("Client: Failed to create socket");
        exit(-1);
    }
    // Set IP address and port
    socklen_t          addr_len=sizeof(addrClient);

    memset(&addrClient, 0, sizeof(addrClient));
    addrClient.sin_family = AF_INET;          // Use IPV4
    addrClient.sin_port   = htons(port_out);
    addrClient.sin_addr.s_addr = inet_addr(mIPListenToServer); // IP address of server
    // set listen interval
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = receiver_interval*1000;  // millisecond to usecond
    setsockopt(mSockfdClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
    // bind ports
    if (bind(mSockfdClient, (struct sockaddr*)&addrClient, addr_len) == -1){
        printf("Client: Failed to bind socket on port %d\n", port_out);
        close(mSockfdClient);
        return false;
    }
    // set buffer
    memset(mbuffer, 0, mBuffersize);

    return false;
}

}
