#ifndef _MODELSOCKET_H_
#define _MODELSOCKET_H_

#include "IModel.h"

//class cData
//{
//public:
//    char data[32]={};
//};

//struct sData
//{
//    char data[32]={};
//};

//state = 0 ~_SELF_CLOSESOCKET
//state = 1 
//state = 2 BLOCK_EXCHANGE_DATA_BEFORE_CLOSESOCKET_VCONNECTION

class ModelSocket : IModel
{
public:
    int type;
    int sizeData = 32;
    std::string ipaddr;
    int port;
    #if TARGET_PLATFORM == PLATFORM_WINDOWS
    WSADATA wsdata;
    SOCKET socket;
    SOCKADDR_IN sockaddr;
    int wsOK;
    #endif
    #if TARGET_PLATFORM == PLATFORM_UNIX
    int socket;
    struct sockaddr_in sockaddr;
    #endif
    int sockaddr_size;
    pthread_t th;
    char cstr[32] = {};
    //char *cstr;
    std::string str;
    std::stringstream sendStream;
    std::stringstream recvStream;
    std::string sendString = " ";
    std::string recvString = " ";
    int state = 0;
    std::string sstate = "default"; 
public:
    ModelSocket(){}
    ~ModelSocket(){}
};
#endif