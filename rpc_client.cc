#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
#include <sstream>
#include "rpc.h"
#include "pmap.h"
//using namespace std;

pmap clientDataBase;


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
        return -1;
    }
    
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if(socketfd == -1){
        std::cerr<<"Socket Error"<<std::endl;
        return -1;
    }
    
    
    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status == -1){
        std::cerr<<"connection error!"<<std::endl;
        return -1;
    }
    
    return socketfd;
}


int ConnectToBinder(){
    int socketfd = -1;
        //connect to binder
#warning Need to change to dynamic address
        std::string name = "127.0.0.1";
        const char* host = name.c_str();//getenv("BINDER_ADDRESS");//get the server hostname from env
        if(host == NULL){
            std::cerr<<"host is null"<<std::endl;
            return -1;
        }
        std::string p = "8888";
        const char* portnum = p.c_str();//getenv("BINDER_PORT");//get the server socket port from env
        if(portnum == NULL){
            std::cerr<<"port number is NULL!"<<std::endl;
            return -1;
        }
        socketfd = Connection(host, portnum);
//        std::cout<<"new socket fd with binder is: "<<socketfd<<std::endl;
        if(socketfd < 0){
            std::cerr<<"wrong socket identifier!"<<std::endl;
            return -1;;
        }
        return socketfd;
}


//**********************************
// clientHandleBinderResponse
//
//**********************************
int clientHandleBinderResponse(int connectionSocket) {
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    uint32_t responseErrorCode_network = 0;
    uint32_t responseErrorCode = 0;
    
    // Receive response type
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive binder response type failed\n");
        return -1;
    }
    receivedSize = -1;
    
    // Receive response error code
    receivedSize = recv(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive binder response errorCode failed\n");
        return -1;
    }
    
    responseType = ntohl(responseType_network);
    responseErrorCode = ntohl(responseErrorCode_network);
//    printf("type: %d, errorCode: %d\n", responseType, responseErrorCode);
    if (responseType == LOC_FAILURE) {
        //perror("Binder response: REGISTER_FAILURE Error Code: %d\n");
        std::cerr << "Binder response: LOC_FAILURE Error Code: " << responseErrorCode << std::endl;
        return responseErrorCode;
    } else if (responseType == LOC_SUCCESS) {
//        printf("Binder response: LOC_SUCCESS\n");
        return 0;
    } else {
        printf("No such a response type from binder to client! Type is: %d\n",responseType);
        return -1;
    }
}




//**********************************
// clientHandleResponse
//
//**********************************
int clientHandleResponse(int connectionSocket) {
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    uint32_t responseErrorCode_network = 0;
    int responseErrorCode = 0;
    
    // Receive response type
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive server response type failed\n");
        return -1;
    }
    receivedSize = -1;
    
    // Receive response error code
    receivedSize = recv(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive server response errorCode failed\n");
        return -1;
    }
    
    responseType = ntohl(responseType_network);
    responseErrorCode = ntohl(responseErrorCode_network);
//    printf("type: %d, errorCode: %d\n", responseType, responseErrorCode);
    if (responseType == EXECUTE_FAILURE) {
        //perror("Binder response: REGISTER_FAILURE Error Code: %d\n");
        std::cerr << "Server response: EXECUTE_FAILURE Error Code: " << responseErrorCode << std::endl;
        return 2;
    } else if (responseType == EXECUTE_SUCCESS) {
//        printf("Server response: EXECUTE_SUCCESS\n");
        return 0;
    } else {
        return 0;
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
        return -1;
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
        perror("Client registion to binder: Send message length failed\n");
        return -1;
    }
//    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client registion to binder: Send message type failed\n");
        return -1;
    }
//    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Client registion to binder: Send message body failed\n");
        return -1;
    }
//    printf("Send message body succeed: %zd\n", operationResult);
    
//    printf("Name: %s\n", name);
//    printf("ArgTypes: ");
//    for (int i = 0; i < argTypesLength(argTypes); i++) {
//        printf("%ud ", argTypes[i]);
//    }
//    printf("\n");

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
    memcpy(messageBody+offset, name, sizeOfName); // [name]
    offset += sizeOfName;
    memcpy(messageBody + offset, &seperator, 1); // [name,]
    offset += 1;
    memcpy(messageBody + offset, argTypes, sizeOfArgTypes); //
    offset += sizeOfArgTypes;
    memcpy(messageBody + offset, &seperator, 1); // [name,argTypes,]
    offset += 1;
    for(int i = 0; i < numofargs - 1; i++){
        int lengthOfArray = argTypes[i] & ARG_ARRAY_LENGTH_MASK;
        // If length is 0, scalar
        if (lengthOfArray == 0) {
            lengthOfArray = 1;
        }
        memcpy(messageBody + offset, args[i], lengthOfArray * argSize(argTypes[i]));
//        std::cout<<i<<" th arg size is "<<argSize(argTypes[i])<<std::endl;
        offset += lengthOfArray * argSize(argTypes[i]);
    }
    memcpy(messageBody + offset, &seperator, 1);
    
//    printf("total message size: %d\n",totalSize);
    uint32_t messageLength = totalSize;
    uint32_t messageType = EXECUTE;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    
    ssize_t operationResult = -1;
    operationResult = send(sockfd, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client request execute to server: Send message length failed\n");
        std::cout<<"expect length is: "<<sizeof(uint32_t)<<" and the actual length is: "<<operationResult<<std::endl;
        return -1;
    }
    //printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client request execute to server: Send message type failed\n");
        return -1;
    }
    //printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Client request execute to server: Send message body failed\n");
        return -1;
    }
    //printf("Send message body succeed: %zd\n", operationResult);
    
//    printf("Name: %s\n", name);
//    printf("ArgTypes: ");
//    for (int i = 0; i < argTypesLength(argTypes); i++) {
//        printf("%s\n", u32ToBit(argTypes[i]));
//    }
//    printf("\n");
    
    //printOutArgs(argTypes, args);
    
    
    int response = 0;
    response = clientHandleResponse(sockfd);
    if(response == 0){
        //success
        // Allocate messageBody
        BYTE *messageBody = (BYTE *)malloc(sizeof(BYTE) * messageLength);
        
        // Receive message body
        int receivedSize = -1;
        receivedSize = (int)recv(sockfd, messageBody, messageLength, 0);
        if (receivedSize != messageLength) {
            perror("Client received wrong length of message body\n");
            return -1;
        }
        //printf("Received length of message body: %zd\n", receivedSize);
        
        // Message body: [name,argTypes,argsByte,]
        char *name_received = NULL;
        int *argTypes_received = NULL;
        BYTE *argsByte_received = NULL; //expanded args
        //void ** args_received = NULL;
        int lastSeperatorIndex = -1;
        int messageCount = 0;
        
        // Process message body, initialize ip, port, name, argTypes
        for (int i = 0; i < receivedSize; i++) {
            if (messageBody[i] == ',') {
                switch (messageCount) {
                    case 0: {
                        uint32_t sizeOfName = i - (lastSeperatorIndex + 1);
                        name_received = (char *)malloc(sizeof(char) * sizeOfName);
                        memcpy(name_received, messageBody + lastSeperatorIndex + 1, sizeOfName);
                        break;
                    }
                    case 1: {
                        uint32_t sizeOfArgTypes = i - (lastSeperatorIndex + 1);
                        argTypes_received = (int *)malloc(sizeof(BYTE) * sizeOfArgTypes);
                        memcpy(argTypes_received, messageBody + lastSeperatorIndex + 1, sizeOfArgTypes);
                        break;
                    }
                    case 2: {
                        uint32_t sizeOfArgsByte = i - (lastSeperatorIndex + 1);
                        assert(sizeOfArgsByte == argsSize(argTypes_received));
                        argsByte_received = (BYTE *)malloc(sizeof(BYTE) * sizeOfArgsByte);
                        memcpy(argsByte_received, messageBody + lastSeperatorIndex + 1, sizeOfArgsByte);
                        break;
                    }
                    default:
                        perror("Message Body Error\n");
                        free(name_received);
                        free(argTypes_received);
                        free(argsByte_received);
//                        free(args_received);
                        return -1;
                        break;
                }
                lastSeperatorIndex = i;
                messageCount++;
            }
        }
        
//        printf("Execute Name: %s\n", name_received);
//        printf("Execute ArgTypes: ");
//        for (int i = 0; i < argTypesLength(argTypes_received); i++) {
//            printf("%s\n", u32ToBit(argTypes_received[i]));
//        }
//        printf("\n");
        if(argsByte_received == NULL){
            printf("args byte is null\n");
        }
        //printOutArgsByte(argTypes_received, argsByte_received);
        
        
        // Process for args from argsByte, comsumes (int* argTypes, void** args == NULL,
        // BYTE *argsByte)
        if (argsByteToArgs(argTypes_received, argsByte_received, args)) {
//            printf("args init succeed!\n");
        } else {
            free(name_received);
            free(argTypes_received);
            free(argsByte_received);
//            free(args_received);
            return -1;
        }
        //printOutArgs(argTypes_received, args);
//        args = args_received;
    }
    else{
        printf("reasonCode: %d\n",response);
        return response;
    }
    
    return 0;
}



int terminateRequest(int sockfd){
    //error checking?
    // Prepare message content
    
    
    BYTE* messageBody = NULL;
    
    // Prepare first 8 bytes: Length(4 bytes) + Type(4 bytes)
    uint32_t messageLength = 0;
    uint32_t messageType = TERMINATE;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    // Send message length (4 bytes)
    ssize_t operationResult = -1;
    operationResult = send(sockfd, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client termination message to binder: Send message length failed\n");
        return -1;
    }
//    printf("Send termination message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client termination message to binder: Send message type failed\n");
        return -1;
    }
//    printf("Send termination message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Client termination message to binder: Send message body failed\n");
        return -1;
    }
//    printf("Send termination message body succeed: %zd\n", operationResult);
    
    return 0;
}

/******************* Client Cached Functions ****************
 *
 *  Description: Cached functions for client requests
 *
 ************************************************************/

int CachedLocationRequest(char* name, int* argTypes, int sockfd){
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
    uint32_t messageType = LOC_CACHED_REQUEST;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    // Send message length (4 bytes)
    ssize_t operationResult = -1;
    operationResult = send(sockfd, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client sends requests to binder: Send message length failed\n");
        return -1;
    }
    //    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Client sends requests to binder: Send message type failed\n");
        return -1;
    }
    //    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(sockfd, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Client sends requests to binder: Send message body failed\n");
        return -1;
    }
    //    printf("Send message body succeed: %zd\n", operationResult);
    
    //    printf("Name: %s\n", name);
    //    printf("ArgTypes: ");
    //    for (int i = 0; i < argTypesLength(argTypes); i++) {
    //        printf("%ud ", argTypes[i]);
    //    }
    //    printf("\n");
    
    return 0;
}


int clientHandleBinderResponseForCachedCall(int connectionSocket) {
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    uint32_t responseErrorCode_network = 0;
    uint32_t responseErrorCode = 0;
    
    // Receive response type
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive binder response type failed\n");
        return -1;
    }
    receivedSize = -1;
    
    // Receive response error code
    receivedSize = recv(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Client receive binder response errorCode failed\n");
        return -1;
    }
    
    responseType = ntohl(responseType_network);
    responseErrorCode = ntohl(responseErrorCode_network);
    //    printf("type: %d, errorCode: %d\n", responseType, responseErrorCode);
    if (responseType == LOC_CACHED_FAILURE) {
        //perror("Binder response: REGISTER_FAILURE Error Code: %d\n");
        std::cerr << "Binder response: LOC_FAILURE Error Code: " << responseErrorCode << std::endl;
        return responseErrorCode;
    } else if (responseType == LOC_CACHED_SUCCESS) {
        //        printf("Binder response: LOC_SUCCESS\n");
        return 0;
    } else {
        printf("No such a response type from binder to client! Type is: %d\n",responseType);
        return -1;
    }
}

int getServerSocket(char* name, int* argTypes,int binder_fd){
    //get the ip and port from binder
    P_NAME_TYPES newKey = P_NAME_TYPES(name,argTypes);
    ssize_t receivedSize = -1;
    uint32_t messageLength_network;
    receivedSize = recv(binder_fd, &messageLength_network, sizeof(uint32_t), 0);//receive length
    if (receivedSize == 0) {
        perror("Cached Call: received wrong message length\n");
        close(binder_fd);
        return -1;
    }
    uint32_t messageLength = ntohl(messageLength_network);
    if(messageLength%20 != 0){
        perror("The length from binder is correct\n");
        return -1;
    }
    std::cout<<"the length of message is "<<messageLength<<std::endl;
    
    BYTE* message_body = (BYTE *)malloc(sizeof(BYTE) * messageLength);
    receivedSize = recv(binder_fd,message_body , messageLength, 0);
    // Connection lost
    if (receivedSize == 0) {
        perror("Connection between client and binder is lost\n");
        close(binder_fd);
        return -1;
    }
    else if (receivedSize != messageLength) {
        perror("Client received wrong length of message length\n");
        close(binder_fd);
        return -1;
    }
    else {
        std::cout<<"received size is "<<receivedSize<<std::endl;
        close(binder_fd);
    }
    int NumOfServers = messageLength/20;
    char* server_host = NULL;
    int portnum;
    int offset = 0;
    
    std::cout<<"inserting server info "<<std::endl;
    for(int i = 0; i < NumOfServers; i++){
        server_host = (char*)malloc(sizeof(sizeof(char) * 16));
        memcpy(server_host,message_body+offset,16);
        offset += 16;
        memcpy(&portnum, message_body+offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        P_IP_PORT newIpPort = P_IP_PORT(server_host,portnum);
        clientDataBase.insert(newKey, newIpPort);
    }
    
    P_IP_PORT* serverIpPort = clientDataBase.findIp_cached(newKey);
    if(serverIpPort == NULL){
        perror("server Ip Port pair cannot be null!\n");
        return -1;
    }
    portnum = serverIpPort->second;
    std::ostringstream convert;
    convert << portnum;
    std::string s = convert.str();
    const char* server_port = s.c_str();
    
    int server_sockfd;
    std::cout<<"server IP address: "<<server_host<<" server port: "<<server_port<<std::endl;
    server_sockfd = Connection(server_host, server_port);
    std::cout<<"Cached Call: server socket fd is: "<<server_sockfd<<std::endl;
    free(serverIpPort);
    serverIpPort = NULL;
    if(server_sockfd < 0){
        std::cerr<<"Server Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    return server_sockfd;
}



/******************* Client Functions ****************
 *
 *  Description: Client related functions
 *
 ****************************************************/


int rpcCall(char* name, int* argTypes, void** args) {
    std::cout<<std::endl;
    std::cout<<"***********************************"<<std::endl;
    std::cout << "rpcCall(" << name << ")" << std::endl;
    int binder_fd;
    binder_fd = ConnectToBinder();//connect to binder first
    if(binder_fd < 0){
        std::cerr<<"Binder Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    int reasoncode = locationRequest(name, argTypes,binder_fd);
    if(reasoncode != 0){
        perror("failed to send request to binder\n");
        return reasoncode;
    }
    
    reasoncode = clientHandleBinderResponse(binder_fd);
    if(reasoncode != 0){
        perror("failed to get response connect to binder\n");
        return reasoncode;
    }
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
        close(binder_fd);
        return -1;
    }
    else {
        // Received message length should be 16 + sizeof(portNumber) + 2
        // Receive message length correctly
//        printf("Received length of message length: %zd\n", receivedSize);
        close(binder_fd);
    }
    char* server_host = (char*)malloc(sizeof(sizeof(char) * 16));
    memcpy(server_host,message_body,16);
    int portnum;
    memcpy(&portnum, message_body+17, sizeof(int));//16 th is a seperator, so go from 17
    std::ostringstream convert;
    convert << portnum;
    std::string s = convert.str();
    const char* server_port = s.c_str();

    int server_sockfd;
//    std::cout<<"server IP address: "<<server_host<<" server port: "<<server_port<<std::endl;
    server_sockfd = Connection(server_host, server_port);
//    std::cout<<"server socket fd is: "<<server_sockfd<<std::endl;
    if(server_sockfd < 0){
        std::cerr<<"Server Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    reasoncode = executeRequest(name, argTypes, args, server_sockfd);
    if(reasoncode != 0){
        close(server_sockfd);
        return reasoncode;
    }
    close(server_sockfd);
    return 0;
}
int rpcCacheCall(char* name, int* argTypes, void** args) {
    std::cout<<std::endl;
    std::cout<<"***********************************"<<std::endl;
    std::cout << "rpcCacheCall(" << name << ")" << std::endl;
    
    P_NAME_TYPES key(name,argTypes);
    P_IP_PORT* serverIpPort = NULL;
    serverIpPort = clientDataBase.findIp_cached(key);
    P_IP_PORT IpPort;
    if(serverIpPort != NULL) IpPort = *serverIpPort;
    while(serverIpPort != NULL){
        int portnum = serverIpPort->second;
        char* server_host = serverIpPort->first;
        std::ostringstream convert;
        convert << portnum;
        std::string s = convert.str();
        const char* server_port = s.c_str();
        
        int server_sockfd;
        std::cout<<"server IP address: "<<server_host<<" server port: "<<server_port<<std::endl;
        server_sockfd = Connection(server_host, server_port);
        std::cout<<"Cached Call: server socket fd is: "<<server_sockfd<<std::endl;
        if(server_sockfd < 0){
            std::cerr<<"Server Connection Error Occur!"<<std::endl;
            return -1;
        }

        int reason = executeRequest(name, argTypes, args, server_sockfd);
        if(reason != 0){
            close(server_sockfd);
            if(serverIpPort != NULL){
                free(serverIpPort);
                serverIpPort = NULL;
            }
            serverIpPort = clientDataBase.findIp_cached(key);
            if(clientDataBase.isIpPortEqual(*serverIpPort, IpPort)){
                clientDataBase.clear_vecIpForCached(key);
                serverIpPort = NULL;
                break;
            }
            continue;
        }
        close(server_sockfd);
        return 0;
    }
    int binder_fd;
    binder_fd = ConnectToBinder();//connect to binder first
    if(binder_fd < 0){
        std::cerr<<"Binder Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    std::cout<<"send cache loc request"<<std::endl;
    int reasoncode = CachedLocationRequest(name, argTypes,binder_fd);
    if(reasoncode != 0){
        perror("CachedCall: failed to send location request to binder\n");
        return reasoncode;
    }
    
    std::cout<<"waiting binder response"<<std::endl;
    reasoncode = clientHandleBinderResponseForCachedCall(binder_fd);
    if(reasoncode != 0){
        perror("CachedCall: failed to handle response from binder\n");
        return reasoncode;
    }
    
    int server_sockfd;
    //    std::cout<<"server IP address: "<<server_host<<" server port: "<<server_port<<std::endl;
    server_sockfd = getServerSocket(name, argTypes,binder_fd);
        std::cout<<"server socket fd is: "<<server_sockfd<<std::endl;
    if(server_sockfd < 0){
        std::cerr<<"Server Connection Error Ocurrs!"<<std::endl;
        return -1;
    }
    
    reasoncode = executeRequest(name, argTypes, args, server_sockfd);
    if(reasoncode != 0){
        close(server_sockfd);
        return reasoncode;
    }
    close(server_sockfd);
    return 0;
}


int rpcTerminate() {
    std::cout << "rpcTerminate()" << std::endl;
    int binder_fd;
    binder_fd = ConnectToBinder();//connect to binder first
    if(binder_fd < 0){
        std::cerr<<"Binder Connection Error Ocurrs!"<<std::endl;
        return -1;
    }

    int result = terminateRequest(binder_fd);
    if(result != 0){
        perror("terminate failed!\n");
        return -1;
    }
    close(binder_fd);
    return 0;
}





