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
    mClientIP = server_addr.c_str();
    mTimeOutMax = fs["timeout_max"];

    CreateServerSocket();
    CreateClientSocket();

}

void udpSocket::RunServer()
{
    int counter = 0;
    socklen_t  addr_len=sizeof(addrServer);
    while(1){

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

void udpSocket::RunClient()
{
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
    printf("No data is received from port: %d \n", mPortOut);
    close(mSockfdClient);
}

bool udpSocket::CreateServerSocket(){
    // create socket
    mSockfdServer = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSockfdServer == -1){
        puts("Server: Failed to create server socket");
        exit(-1);
    }
    // Set IP address and port
    socklen_t  addr_len=sizeof(addrServer);
    memset(&addrServer, 0, sizeof(addrServer));

    addrServer.sin_family = AF_INET;
    addrServer.sin_port   = htons(mPortOut);
    addrServer.sin_addr.s_addr = inet_addr(mClientIP);

    puts("Server is established!");
    return true;
}

bool udpSocket::CreateClientSocket(){
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
    addrClient.sin_port   = htons(mPortOut);
    addrClient.sin_addr.s_addr = htonl(INADDR_ANY);

    // set listen interval
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = mReceiverInterval*1000;  // millisecond to usecond
    setsockopt(mSockfdClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // bind ports
    if (bind(mSockfdClient, (struct sockaddr*)&addrClient, addr_len) == -1){
        printf("Client: Failed to bind socket on port %d\n", mPortOut);
        close(mSockfdClient);
        return false;
    }
    // set buffer
    memset(mbuffer, 0, mBuffersize);

    puts("Client is listening!");
    return true;
}

}
