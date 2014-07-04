#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include "rpc.h"
//using namespace std;


/***************** connect *************************
 Purpose: connect to binder by using hostname and
          port number
 
 return: (integer) the socket fd or -1 as error occurs
 ********************************************************/
int Connection(char* hostname, char* portnumber){
    struct addrinfo host_info;
    struct addrinfo* host_info_list;
    int status;
    
    memset(&host_info, 0, sizeof host_info);
    
    host_info.ai_family = AF_UNSPEC; //IP version not specified.
    host_info.ai_socktype = SOCK_STREAM;//SOCK_STREAM is for TCP
    status = getaddrinfo(hostname,portnumber, &host_info, &host_info_list);
    if(status){
        std::cerr<<"Cannot Get Address Information!"<<std::endl;
        exit(-1);
    }
    
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if(socketfd == -1){
        std::cerr<<"Socket Error"<<std::endl;
        exit(-1);
    }
    
    
    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status == -1){
        std::cerr<<"connection error!"<<std::endl;
        exit(-1);
    }
    
    return socketfd;
}

int socketfd = -1;

int ConnectToBinder(){
    if(socketfd < 0){
        //connect to binder
        char* host = getenv("BINDER_ADDRESS");//get the server hostname from env
        if(host == NULL){
            std::cerr<<"host is null"<<std::endl;
            exit(-1);
        }
        char* portnum = getenv("BINDER_PORT");//get the server socket port from env
        if(portnum == NULL){
            std::cerr<<"port number is NULL!"<<std::endl;
            exit(-1);
        }
        socketfd = Connection(host, portnum);
        if(socketfd < 0){
            std::cerr<<"wrong socket identifier!"<<std::endl;
            exit(-1);
        }
        return socketfd;
    }
    else{
        //already connected to binder, return the socket descriptor
        return socketfd;
    }
}

int locationRequest(char* name, int* argTypes, int sockfd){
    
    unsigned int remained_size;
    int sent_size;
    
    while(remained_size > 0){
        //sent_size = send(sockfd, message, message.size()+1, 0);
        if(sent_size == 0){
            break;
        }
        else if(sent_size < 0){
            return sent_size;
        }
        else{
            remained_size -= sent_size;
        }
    }
    // Prepare message content
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t totalSize = sizeOfName + sizeOfArgTypes + 4;
    //    printf("%d + 1 + %d + 1 + %d + 1 + %d + 1 = %d\n", sizeOfIp, sizeOfPort, sizeOfName, sizeOfArgTypes, totalSize);
    

    BYTE messageBody[totalSize];
    char seperator = ',';
    memcpy(messageBody, name, sizeOfName); // [ip,portnum,name]
    memcpy(messageBody + sizeOfName, &seperator, 1); // [ip,portnum,name,]
    memcpy(messageBody + sizeOfName + 1, argTypes, sizeOfArgTypes); // [ip,portnum,name,argTypes]
    memcpy(messageBody + sizeOfName + 1 + sizeOfArgTypes, &seperator, 1); // [ip,portnum,name,argTypes,]
    
    // Prepare first 8 bytes: Length(4 bytes) + Type(4 bytes)
    uint32_t messageLength = totalSize;
    uint32_t messageType = LOC_REQUEST;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    // Send message length (4 bytes)
    ssize_t operationResult = -1;
    operationResult = send(sockfd, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server registion to binder: Send message length failed\n");
        return -1;
    }
    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server registion to binder: Send message type failed\n");
        return -1;
    }
    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Server registion to binder: Send message body failed\n");
        return -1;
    }
    printf("Send message body succeed: %zd\n", operationResult);
    

    return 0;
}

/******************* Client Functions ****************
 *
 *  Description: Client related functions
 *
 ****************************************************/


int rpcCall(char* name, int* argTypes, void** args) {
    std::cout << "rpcCall(" << name << ")" << std::endl;
    int fd;
    fd = ConnectToBinder();//connect to binder first
    if(fd < 0){
        std::cerr<<"Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    locationRequest(name, argTypes,fd);
    
    //sending message
    return 0;
}
int rpcCacheCall(char* name, int* argTypes, void** args) {
    std::cout << "rpcCacheCall(" << name << ")" << std::endl;
    return 0;
}
int rpcTerminate() {
    std::cout << "rpcTerminate()" << std::endl;
    return 0;
}





