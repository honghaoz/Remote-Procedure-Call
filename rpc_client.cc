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
int Connection(const char* hostname, const char* portnumber){
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
#warning Need to change to dynamic address
        std::string name = "127.0.0.1";
        const char* host = name.c_str();//getenv("BINDER_ADDRESS");//get the server hostname from env
        if(host == NULL){
            std::cerr<<"host is null"<<std::endl;
            exit(-1);
        }
        std::string p = "8888";
        const char* portnum = p.c_str();//getenv("BINDER_PORT");//get the server socket port from env
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

/***************** connect to server *************************
 Purpose: 
 
 return: (integer) the socket fd or -1 as error occurs
 ********************************************************/
int ConnectToServer(char* hostname, char* portnumber){
    int sockfd;
    sockfd = Connection(hostname, portnumber);
    if(sockfd < 0){
        std::cerr<<"wrong socket identifier!"<<std::endl;
        exit(-1);
    }
    return sockfd;
    
}

int locationRequest(char* name, int* argTypes, int sockfd){
    //error checking?
    if(name == NULL){
        perror("null pointer of binder ip");
        return -1;
    }
    if(argTypes == NULL){
        perror("null pointer of argtype");
        return -1;
    }
    // Prepare message content
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    char seperator = ',';
    uint32_t sizeOfSeperator = sizeof(seperator);
    uint32_t totalSize = sizeOfName + sizeOfArgTypes + 2 * sizeOfSeperator;
    //    printf("%d + 1 + %d + 1 + %d + 1 + %d + 1 = %d\n", sizeOfIp, sizeOfPort, sizeOfName, sizeOfArgTypes, totalSize);
    

    BYTE messageBody[totalSize];
    int offset = 0;
    memcpy(messageBody+offset, name, sizeOfName); // [ip,portnum,name]
    offset += sizeOfName;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,portnum,name,]
    offset += sizeOfSeperator;
    memcpy(messageBody + offset, argTypes, sizeOfArgTypes); // [ip,portnum,name,argTypes]
    offset += sizeOfArgTypes;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,portnum,name,argTypes,]
    
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
    
    printf("Name: %s\n", name);
    printf("ArgTypes: ");
    for (int i = 0; i < argTypesLength(argTypes); i++) {
        printf("%ud ", argTypes[i]);
    }
    printf("\n");

    return 0;
}


int executeRequest(char* name, int* argTypes, void** args, int sockfd){
    if(name == NULL ){
        perror("null pointer of binder ip");
        return -1;
    }
    if(argTypes == NULL){
        perror("null pointer of argTypes");
        return -1;
    }
    if(args == NULL){
        perror("null pointer of arguments");
        return -1;
    }
    
    int numofargs = argTypesLength(argTypes); // number of args
    
    // Prepare message content
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = numofargs * sizeof(int);
    uint32_t sizeOfargs = argsSize(argTypes); //get the total size of all args
    uint32_t totalSize = sizeOfName + sizeOfArgTypes + sizeOfargs + 3;
    
    
    BYTE messageBody[totalSize];
    char seperator = ',';
    int offset = 0;
    memcpy(messageBody+offset, name, sizeOfName); // [ip,portnum,name]
    offset += sizeOfName;
    memcpy(messageBody + offset, &seperator, 1); // [ip,portnum,name,]
    offset += 1;
    memcpy(messageBody + offset, argTypes, sizeOfArgTypes); //
    offset += sizeOfArgTypes;
    memcpy(messageBody + offset, &seperator, 1); // [ip,portnum,name,argTypes,]
    offset += 1;
    for(int i = 0; i < numofargs - 1; i++){
        int lengthOfArray = argTypes[i] & ARG_ARRAY_LENGTH_MASK;
        // If length is 0, scalar
        if (lengthOfArray == 0) {
            lengthOfArray = 1;
        }
        memcpy(messageBody + offset, args[i], lengthOfArray * argSize(argTypes[i]));
        
        offset += lengthOfArray * argSize(argTypes[i]);
    }
    memcpy(messageBody + offset, &seperator, 1);
    
    
    uint32_t messageLength = totalSize;
    uint32_t messageType = EXECUTE;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    
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
    
    printf("Name: %s\n", name);
    printf("ArgTypes: ");
    for (int i = 0; i < argTypesLength(argTypes); i++) {
        printf("%s\n", u32ToBit(argTypes[i]));
    }
    printf("\n");
    printOutArgs(argTypes, args);
    
    return 0;
}


/******************* Client Functions ****************
 *
 *  Description: Client related functions
 *
 ****************************************************/


int rpcCall(char* name, int* argTypes, void** args) {
    std::cout << "rpcCall(" << name << ")" << std::endl;
    int binder_fd, server_fd;
    binder_fd = ConnectToBinder();//connect to binder first
    if(binder_fd < 0){
        std::cerr<<"Binder Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    locationRequest(name, argTypes,binder_fd);
    
    //get the ip and port from binder
    ssize_t receivedSize = -1;
    char seperator = ',';
    uint32_t sizeOfSeperator = sizeof(seperator);
    uint32_t messageLength = 16 + sizeof(int) + 2 * sizeOfSeperator;//the total size of message from binder
    BYTE* message_body = (BYTE *)malloc(sizeof(BYTE) * messageLength);
    receivedSize = recv(binder_fd,message_body , messageLength, 0);
    // Connection lost
    if (receivedSize == 0) {
        perror("Connection between client and binder is lost\n");
        close(binder_fd);
        return -1;
    }
    else if (receivedSize != messageLength) {
        perror("Binder received wrong length of message length\n");
        return -1;
    }
    else {
        // Received message length should be 16 + sizeof(portNumber) + 2
        // Receive message length correctly
        printf("Received length of message length: %zd\n", receivedSize);
    }
    char* server_host = (char*)malloc(sizeof(sizeof(char) * 16));
    memcpy(server_host,message_body,16);
    int portnum;
    memcpy(&portnum, message_body+17, sizeof(int));
    std::string s = std::to_string(portnum);
    const char* server_port = s.c_str();

    int server_sockfd;
    std::cout<<"server IP address: "<<server_host<<" server port: "<<server_port<<std::endl;
    server_sockfd = Connection(server_host, server_port);
    if(server_fd < 0){
        std::cerr<<"Server Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    executeRequest(name, argTypes, args, server_sockfd);
    
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





