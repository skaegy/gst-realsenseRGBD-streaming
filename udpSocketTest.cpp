//
// Created by root on 31/08/18.
//

#include "udpSocketSimple.h"
#include <cstring>
#include <iostream>
#include <thread>
#include <pthread.h>

skaegy::udpSocket* UdpSocket;
std::thread* mptUdpSocketServer;
std::thread* mptUdpSocketClient;
int main(){
    UdpSocket = new skaegy::udpSocket("../settings.yaml");
    //mptUdpSocketServer = new thread(&skaegy::udpSocket::RunServer, mpUdpSocket);
    mptUdpSocketClient = new thread(&skaegy::udpSocket::Run, UdpSocket);
}