//
// Created by skaegy on 31/08/18.
//

#ifndef RGBDSTREAMING_UDPSOCKETSIMPLE_H
#define RGBDSTREAMING_UDPSOCKETSIMPLE_H

#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <mutex>
#include <cstring>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

namespace skaegy{

class udpSocket{

public:
    udpSocket(const string strSettingFile);

    void RunServer();

    void RunClient();

    bool CreateServerSocket();

    bool CreateClientSocket();

    bool mCloseServer = false;

    bool mCloseClient = false;

    bool isServerOk, isClientOk;

private:
    struct sockaddr_in addrServer, addrClient;

    int mPortIn, mPortOut;
    int mReceiverInterval, mSenderInterval;
    const char* mClientIP;
    int mTimeOutMax;

    int mSockfdServer, mSockfdClient;
    int mBuffersize = 1000;
    char mbuffer[1000];

};

}
#endif //RGBDSTREAMING_UDPSOCKETSIMPLE_H
