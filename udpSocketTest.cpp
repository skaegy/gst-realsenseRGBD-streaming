//
// Created by root on 31/08/18.
//

#include "udpSocketSimple.h"
#include <cstring>
#include <iostream>


skaegy::udpSocket* mpUdpSocket;
std::thread* mptUdpSocketServer;
std::thread* mptUdpSocketClient;
int main(){
    mpUdpSocket = new skaegy::udpSocket("../settings.yaml");
    mptUdpSocketServer = new thread(&skaegy::udpSocket::RunServer, mpUdpSocket);
    mptUdpSocketClient = new thread(&skaegy::udpSocket::RunClient, mpUdpSocket);

    while(1){
        int a = 1;
    }
}