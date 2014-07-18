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
#include <utility>
#include <map>
#include "rpc.h"
#include "pmap.h"
//using namespace std;

/******************* Binder Functions ****************
 *
 *  Description: Binder related functions
 *
 ****************************************************/

#define MAX_NUMBER_OF_CONNECTIONS 100

int binderListenSocket; // binder Listening socket
int binderConnections[MAX_NUMBER_OF_CONNECTIONS]; // Array of connected connections
fd_set binderSocketsFD; // Socket file descriptors that we want to wake up for
int binderHighestSocket; // Highest # of socket file descriptor

pmap binderProcedureToID;
#define BINDER_TERMINATE 999

#pragma mark - rpcBinderInit()
/********************** rpcBinderInit() *************************
 *
 *  1, Create binder socket for server and client, and listen it
 *
 ****************************************************************/
/**
 *  Binder Init, create binder listen socket for servers and clients
 *
 *  @return Binder init execution result
 */
int rpcBinderInit() {
    std::cout << "rpcBinderInit()" << std::endl;
    // Get IPv4 address for this host
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    
    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "BINDER ERROR: Get addr info error: %s\n", gai_strerror(status));
        return -1; // ERROR -1: Get addr info error
    }
    
    for(p = res; p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            // Convert the IP to a string and print it:
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            printf("BINDER_ADDRESS %s\n", ipstr);
            break;
        }
    }
    freeaddrinfo(res); // Free the linked list
    
    // Create socket
    binderListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (binderListenSocket == -1) {
        perror("BINDER ERROR: Could not create socket\n");
        return -2; // ERROR -2: Binder create listen socket error
    }
    
    int addr_size;
    setsockopt(binderListenSocket, SOL_SOCKET, SO_REUSEADDR, &addr_size, sizeof(addr_size)); // So that we can re-bind to it without TIME_WAIT problems
    
    // Prepare the sockaddr_in structure
    struct sockaddr_in binder;
    memset(&binder, 0, sizeof(struct sockaddr_in));
    binder.sin_family = AF_INET;
    binder.sin_addr.s_addr = htonl(INADDR_ANY);
#warning Need to change to dynamic port number
    binder.sin_port = htons(8888);//htons(0);
    
    // Bind
    int bindResult = bind(binderListenSocket, (struct sockaddr *)&binder, sizeof(struct sockaddr_in));
    if(bindResult == -1) {
        perror("BINDER ERROR: Listen socket bind failed\n");
        return -3; // ERROR -3: Binder listen socket bind failed
    }
    
    // Listen
    listen(binderListenSocket , MAX_NUMBER_OF_CONNECTIONS);
    
    // Print out port number
    socklen_t addressLength = sizeof(binder);
    if (getsockname(binderListenSocket, (struct sockaddr*)&binder, &addressLength) == -1) {
        perror("BINDER ERROR: Get port error\n");
        return -4; // ERROR -4: Binder get port number error
    }
    printf("BINDER_PORT %d\n", ntohs(binder.sin_port));
    
    // Since there is only one sock, this is the highest
    binderHighestSocket = binderListenSocket;
    // Clear the clients
    memset((char *) &binderConnections, 0, sizeof(binderConnections));
    return 0; // SUCCESS 0: Binder init succeefully
}





#pragma mark - rpcBinderListen()
/********************** rpcBinderListen() *************************
 *
 *  1, Handle message from server and client
 *
 ****************************************************************/
void binderBuildConnectionList();
int binderReadSockets();
int binderHandleNewConnection();
int binderDealWithData(int connectionNumber);

/**
 *  Binder listen for all incoming message
 *
 *  @return Binder listen execution result
 */
int rpcBinderListen() {
    std::cout << "rpcBinderListen()" << std::endl;
    
    int numberOfReadSockets; // Number of sockets ready for reading
    
    // Binder Server loop
    while (1) {
        binderBuildConnectionList();
        //        timeout.tv_sec = 1;
        //        timeout.tv_usec = 0;
        numberOfReadSockets = select(binderHighestSocket + 1, &binderSocketsFD, NULL, NULL, NULL);
        if (numberOfReadSockets < 0) {
            perror("BINDER ERROR: Select error\n");
            return -1; // ERROR -1:
        }
        if (numberOfReadSockets > 0) {
            int readResult = binderReadSockets();
            if (readResult == BINDER_TERMINATE) {
                //return -1;
//                printf("Binder will terminate\n");
                break;
            } else if (readResult != 0) {
                // Some warnnings or errors
                // Do not return, to avoid exit binder
                // Print out warnings or errors
                if (readResult > 0) {
                    printf("BINDER WARNING: %d\n", readResult);
                } else {
                    printf("BINDER ERROR: %d\n", readResult);
                }
            }
        }
    }
    
    close(binderListenSocket);
//    printf("Binder terminated!\n");
    return 0; // SUCCESS
}

/**
 *  Build binder's connection list, added all sockets to binderSocketsFD
 */
void binderBuildConnectionList() {
	// Clears out the fd_set called socks
	FD_ZERO(&binderSocketsFD);
	
    // Adds listening sock to socks
	FD_SET(binderListenSocket, &binderSocketsFD);
	
    // Add the new connection sock to socks
    for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
        if (binderConnections[i] != 0) {
            FD_SET(binderConnections[i], &binderSocketsFD);
            if (binderConnections[i] > binderHighestSocket) {
                binderHighestSocket = binderConnections[i];
            }
        }
    }
}

/**
 *  Binder handle new connection from client or server
 *
 *  @return Handle execution result
 */
int binderHandleNewConnection() {
    int newSock;
    struct sockaddr_in client;
    int addr_size = sizeof(struct sockaddr_in);
    
    // Try to accept a new connection
    newSock = accept(binderListenSocket, (struct sockaddr *)&client, (socklen_t *)&addr_size);
    if (newSock < 0) {
        perror("BINDER ERROR: Accept new connection failed\n");
        return -2; // ERROR -2: Accept new connection failed
    }
    // Add this new client to clients
    for (int i = 0; (i < MAX_NUMBER_OF_CONNECTIONS) && (i != -1); i++) {
        if (binderConnections[i] == 0) {
            //            printf("\nConnection accepted: FD=%d; Slot=%d\n", newSock, i);
            binderConnections[i] = newSock;
            newSock = -1;
            break;
        }
    }
    if (newSock != -1) {
        perror("BINDER ERROR: No room left for new connection\n");
        close(newSock);
        return -3; // ERROR -3: No room left for new connection
    }
    
    return 0;
}

/**
 *  Binder read active sockets
 *
 *  @return Read execution result
 */
int binderReadSockets() {
    // If listening sock is woked up, there is a new client come in
    if (FD_ISSET(binderListenSocket, &binderSocketsFD)) {
        int handleNewConnectionResult = binderHandleNewConnection();
        if(handleNewConnectionResult < 0) {
            return handleNewConnectionResult;
        }
    }
    
    // Check whether there is new data come in
    for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
        if (FD_ISSET(binderConnections[i], &binderSocketsFD)) {
            int binderDealWithDataResult = binderDealWithData(i);
            if (binderDealWithDataResult < 0) {
                return binderDealWithDataResult;
            } else if (binderDealWithDataResult == BINDER_TERMINATE) {
                return BINDER_TERMINATE;
            }
        }
    }
    return 0;
}


int binderResponse(int connectionSocket, messageType responseType, uint32_t errorCode);
// Binder deal with three different message: Register, Locate and Terminate
int binderDealWithRegisterMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithCachedLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithTerminateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize, int connectionNumber);

/**
 *  Receive message length, message type and allocate message body
 *  According message type, call corresponding function
 *
 *  @param connectionNumber the index for active socket in binderConnections
 *
 *  @return Execution result
 */
int binderDealWithData(int connectionNumber) {
    // Get the socket descriptor
    int connectionSocket = binderConnections[connectionNumber];
    
    // Prepare for message length, message type
    uint32_t messageLength_network = 0;
    uint32_t messageType_network = 0;
    uint32_t messageLength = 0;
    uint32_t messageType = 0;
    
    // Receive message length
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &messageLength_network, sizeof(uint32_t), 0);
    // Connection lost
    if (receivedSize == 0) {
//        printf("\nConnection lost: FD=%d;  Slot=%d\n", connectionSocket, connectionNumber);
        close(connectionSocket);
        
        // Set this place to be available
        binderConnections[connectionNumber] = 0;
        // Check whether there is active connections
        for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
            if (binderConnections[i] != 0) {
                return 0; // SUCCEE
            }
        }
        return BINDER_TERMINATE; // TERMINATE
    }
    else if (receivedSize != sizeof(uint32_t)) {
        perror("BINDER ERROR: Message length error\n");
//        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -4; // ERROR -4:
    }
    else { // Receive message length correctly
//        printf("Received length of message length: %zd\n", receivedSize);
        messageLength = ntohl(messageLength_network);
    }
    
    // Receive message type
    receivedSize = -1;
    receivedSize = recv(connectionSocket, &messageType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("BINDER ERROR: Message type length error\n");
//        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -5; // ERROR -5
    } else { // Receive message length correctly
//        printf("Received length of message type: %zd\n", receivedSize);
        messageType = ntohl(messageType_network);
    }
    
    // Allocate messageBody
    BYTE *messageBody = (BYTE *)malloc(sizeof(BYTE) * messageLength);
    
    // Receive message body
    receivedSize = -1;
    receivedSize = recv(connectionSocket, messageBody, messageLength, 0);
    if (receivedSize != messageLength) {
        perror("BINDER ERROR: Message body length error\n");
//        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -6; // ERROR -6
    }
//    printf("Received length of message body: %zd\n", receivedSize);
    
    switch (messageType) {
        case REGISTER: {
            printf("BINDER: Receive REGISTER\n");
            return binderDealWithRegisterMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        case LOC_REQUEST: {
            printf("BINDER: Receive LOC_REQUEST\n");
            return binderDealWithLocateMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        case TERMINATE: {
            printf("BINDER: Receive TERMINATE\n");
            return binderDealWithTerminateMessage(connectionSocket, messageBody, receivedSize, connectionNumber);
            break;
        }
        case LOC_CACHED_REQUEST: {
            printf("BINDER: Receive LOC_CACHED_REQUEST\n");
            return binderDealWithCachedLocateMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        default:
            perror("BINDER ERROR: Received unknown message type\n");
            if (messageBody != NULL) free(messageBody);
            return -7; // ERROR -7
            break;
    }
    // Never goes here
    if (messageBody != NULL) free(messageBody);
    return 0;
}

/**
 *  Binder response responseType and errorCode with connectionSocket
 *
 *  @param connectionSocket Socket descriptor for the connection
 *  @param responseType     Response message type
 *  @param errorCode        Error code
 *
 *  @return errorCode
 */
// Server send back response
int binderResponse(int connectionSocket, messageType responseType, uint32_t errorCode) {
    // Send response message to server
    uint32_t responseType_network = htonl(responseType);
    uint32_t responseErrorCode_network = htonl(errorCode);
    
    // Send response type
    ssize_t operationResult = -1;
    operationResult = send(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("BINDER ERROR: Send response type failed\n");
        return -8; // ERROR -8
    }
    
    // Send response error code
    operationResult = -1;
    operationResult = send(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("BINDER ERROR: Send response errorCode failed\n");
        return -9; // ERROR -9
    }
    //    printf("Binder responseType: %d, errorCode: %d\n", responseType, errorCode);
    return errorCode;
}

/**
 *  Deal with REGISTER message type
 *  Insert new procedure in binder's local database
 *
 *  @param connectionSocket Socket descriptor for binder for servers
 *  @param messageBody      Received message body [ip,portnum,name,argTypes,]
 *  @param messageBodySize  Size of messageBody in byte
 *
 *  @return Execution result (1 for replaced procedure warning)
 */
int binderDealWithRegisterMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize) {
    // Message body: [ip,portnum,name,argTypes,]
    char *ipv4Address = NULL;
    int portNumber = 0;
    char *name = NULL;
    int *argTypes = NULL;
    
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < messageBodySize; i++) {
        if (messageBody[i] == ',') {
            switch (messageCount) {
                case 0: {
                    uint32_t sizeOfIp = i - (lastSeperatorIndex + 1);
                    if (sizeOfIp > 16) {
                        // Size is too large
                        if (ipv4Address != NULL) free(ipv4Address);
                        if (name != NULL) free(name);
                        if (argTypes != NULL) free(argTypes);
                        if (messageBody != NULL) free(messageBody);
                        perror("BINDER ERROR: REGISTER Wrong IP size (>16)\n");// ERROR -10
                        return binderResponse(connectionSocket, REGISTER_FAILURE, -10);
                    }
//                    printf("%d\n", sizeOfIp);
                    ipv4Address = (char*)malloc(sizeof(char) * sizeOfIp);
                    memcpy(ipv4Address, messageBody + lastSeperatorIndex + 1, sizeOfIp);
                    break;
                }
                case 1: {
                    uint32_t sizeOfPort = i - (lastSeperatorIndex + 1);
                    if (sizeOfPort != sizeof(portNumber)) {
                        // Size is too large
                        if (ipv4Address != NULL) free(ipv4Address);
                        if (name != NULL) free(name);
                        if (argTypes != NULL) free(argTypes);
                        if (messageBody != NULL) free(messageBody);
                        perror("BINDER ERROR: REGISTER Wrong port size (!= 4)\n");// ERROR -11
                        return binderResponse(connectionSocket, REGISTER_FAILURE, -11);
                    }
                    memcpy(&portNumber, messageBody + lastSeperatorIndex + 1, sizeOfPort);
                    break;
                }
                case 2: {
                    uint32_t sizeOfName = i - (lastSeperatorIndex + 1);
                    name = (char *)malloc(sizeof(char) * sizeOfName);
                    memcpy(name, messageBody + lastSeperatorIndex + 1, sizeOfName);
                    break;
                }
                case 3: {
                    uint32_t sizeOfArgTypes = i - (lastSeperatorIndex + 1);
                    argTypes = (int *)malloc(sizeof(BYTE) * sizeOfArgTypes);
                    memcpy(argTypes, messageBody + lastSeperatorIndex + 1, sizeOfArgTypes);
                    break;
                }
                default:
                    if (ipv4Address != NULL) free(ipv4Address);
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    if (messageBody != NULL) free(messageBody);
                    perror("BINDER ERROR: REGISTER Wrong message body\n");// ERROR -12
                    return binderResponse(connectionSocket, REGISTER_FAILURE, -12);
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    // Process completed
    printf("    Registered: IP: %s, Port: %d, Name: %s\n", ipv4Address, portNumber, name);
    
    // If message is correct, register in local database
    P_NAME_TYPES procedureNameTypes(name, argTypes);
    P_NAME_TYPES_SOCKET procedureKey(procedureNameTypes, connectionSocket);
    P_IP_PORT ID(ipv4Address, portNumber);
    int insertResult = binderProcedureToID.insert(procedureKey, ID);
    
    // Clean
    if (messageBody != NULL) free(messageBody);
    
    // Send back register success (0: success, 1: replaced)
    return binderResponse(connectionSocket, REGISTER_SUCCESS, insertResult);
}

/**
 *  Use received message to find an available server, response the IP and Port number
 *
 *  @param connectionSocket Socket descriptor for binder for client
 *  @param messageBody      [name,argTypes,]
 *  @param messageBodySize  size for [name,argTypes,]
 *
 *  @return Execute result
 */
int binderDealWithLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize) {
    // Message body: [name,argTypes,]
    char *name = NULL;
    int *argTypes = NULL;
    char *ipv4Address = NULL;
    int portNumber = 0;
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < messageBodySize; i++) {
        if (messageBody[i] == ',') {
            switch (messageCount) {
                case 0: {
                    uint32_t sizeOfName = i - (lastSeperatorIndex + 1);// get the size of function name
                    name = (char *)malloc(sizeof(char) * sizeOfName);
                    memcpy(name, messageBody + lastSeperatorIndex + 1, sizeOfName);// get the name of function
                    break;
                }
                case 1: {
                    uint32_t sizeOfArgTypes = i - (lastSeperatorIndex + 1);// get the size of function arg type
                    argTypes = (int *)malloc(sizeof(BYTE) * sizeOfArgTypes);
                    memcpy(argTypes, messageBody + lastSeperatorIndex + 1, sizeOfArgTypes);// get the arg Type
                    break;
                }
                default:
                    if (ipv4Address != NULL) free(ipv4Address);
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    if (messageBody != NULL) free(messageBody);
                    perror("BINDER ERROR: LOC Wrong message body\n");// ERROR -13
                    return binderResponse(connectionSocket, LOC_FAILURE, -13);
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    
    P_NAME_TYPES queryKey(name, argTypes);
    P_IP_PORT *queryResult = binderProcedureToID.findIp_client(queryKey);
    if (queryResult == NULL) {
        if (ipv4Address != NULL) free(ipv4Address);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (messageBody != NULL) free(messageBody);
        perror("BINDER ERROR: LOC Procedure not found\n");// ERROR -14
        return binderResponse(connectionSocket, LOC_FAILURE, -14);
    }
    ipv4Address = queryResult->first;
    portNumber = queryResult->second;
    printf("    Locate: %s, found IP: %s, Port: %d\n", name, ipv4Address, portNumber);
    
    // Response LOC_SUCCESS
    binderResponse(connectionSocket, LOC_SUCCESS, 0);
    
    // Send back founded IP and Port
    // Prepare message body: [ip,portnum,]
    uint32_t sizeOfIp = 16; // the fixed length of IP address; define to be IP_SIZE later maybe
    uint32_t sizeOfPort = sizeof(portNumber);
    char seperator = ',';
    uint32_t sizeOfSeperator = sizeof(seperator);
    uint32_t totalSize = sizeOfIp + sizeOfPort + 2 * sizeOfSeperator;
    BYTE messageBodyResponse[totalSize];
    int offset = 0;
    memcpy(messageBodyResponse + offset, ipv4Address, sizeOfIp); // [ip]
    offset += sizeOfIp;
    memcpy(messageBodyResponse + offset, &seperator, sizeOfSeperator); // [ip,]
    offset += sizeOfSeperator;
    memcpy(messageBodyResponse + offset, &portNumber, sizeOfPort); // [ip,portnum]
    offset += sizeOfPort;
    memcpy(messageBodyResponse + offset, &seperator, sizeOfSeperator); // [ip,portnum,]
    offset += sizeOfSeperator;
    
    // Send message body
    ssize_t operationResult = -1;
    operationResult = send(connectionSocket, &messageBodyResponse, totalSize, 0);
    if (operationResult != totalSize) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (messageBody != NULL) free(messageBody);
        perror("BINDER ERROR: LOC Send IP+Port failed\n");// ERROR -15
        return -15;
    }
//    printf("Response located IP: %s\n", ipv4Address);
//    printf("Response located Port: %d\n", portNumber);
    
    if (name != NULL) free(name);
    if (argTypes != NULL) free(argTypes);
    if (messageBody != NULL) free(messageBody);
    return 0;
}

/**
 *  Use received message to find all available servers, response a list of IPs and Port numbers
 *
 *  @param connectionSocket Socket descriptor for binder for client
 *  @param messageBody      [name,argTypes,]
 *  @param messageBodySize  size for [name,argTypes,]
 *
 *  @return Execute result
 */
int binderDealWithCachedLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize) {
    // Message body: [name,argTypes,]
    char *name = NULL;
    int *argTypes = NULL;
    
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < messageBodySize; i++) {
        if (messageBody[i] == ',') {
            switch (messageCount) {
                case 0: {
                    uint32_t sizeOfName = i - (lastSeperatorIndex + 1);// get the size of function name
                    name = (char *)malloc(sizeof(char) * sizeOfName);
                    memcpy(name, messageBody + lastSeperatorIndex + 1, sizeOfName);// get the name of function
                    break;
                }
                case 1: {
                    uint32_t sizeOfArgTypes = i - (lastSeperatorIndex + 1);// get the size of function arg type
                    argTypes = (int *)malloc(sizeof(BYTE) * sizeOfArgTypes);
                    memcpy(argTypes, messageBody + lastSeperatorIndex + 1, sizeOfArgTypes);// get the arg Type
                    break;
                }
                default:
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    if (messageBody != NULL) free(messageBody);
                    perror("BINDER ERROR: LOC_CACHED Wrong message body\n");// ERROR -16
                    return binderResponse(connectionSocket, LOC_CACHED_FAILURE, -16);
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    
    P_NAME_TYPES queryKey(name, argTypes);

    std::vector<P_IP_PORT> queryResult = binderProcedureToID.findIpList_client(queryKey);
    // If no any ip/port found
    if (queryResult.size() == 0) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (messageBody != NULL) free(messageBody);
        perror("BINDER ERROR: LOC_CACHED Procedure not found\n");// ERROR -17
        return binderResponse(connectionSocket, LOC_CACHED_FAILURE, -17);
    }
    
    // Send back founded IP/Ports
    // queryResult contains a list of ip/ports
    ssize_t numberOfIpPorts = queryResult.size();
    binderResponse(connectionSocket, LOC_CACHED_SUCCESS, 0);
    
    // Prepare messageLength
    uint32_t messageLength = (uint32_t)numberOfIpPorts * (16 + sizeof(uint32_t)); // 16bytes for ip, 4 byte for portNumber
    uint32_t messageLength_network = htonl(messageLength);
    
    // Prepare messageBody [length|IP1PORT1 IP2PORT2...]
    BYTE messageBodyResponse[messageLength];
    int offset = 0;
    for (int i = 0; i < numberOfIpPorts; i++) {
        // Copy each Ip + port
        P_IP_PORT eachIpPort = queryResult.at(i);
        memcpy(messageBodyResponse + offset, eachIpPort.first, 16);
        offset += 16;
        memcpy(messageBodyResponse + offset, &eachIpPort.second, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    // Send message length (4 bytes)
    ssize_t operationResult = -1;
    operationResult = send(connectionSocket, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (messageBody != NULL) free(messageBody);
        perror("BINDER ERROR: LOC_CACHED Send Ip+Ports length error\n");
        return -18; // ERROR -18
    }
//    printf("Binder cached call send message length succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(connectionSocket, &messageBodyResponse, messageLength, 0);
    if (operationResult != messageLength) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (messageBody != NULL) free(messageBody);
        perror("BINDER ERROR: LOC_CACHED Send Ip+Ports body failed\n");
        return -19; // ERROR -19
    }
//    printf("Binder cached call send message body succeed: %zd\n", operationResult);
    return 0; //SUCCESS
}

/**
 *  Hanlde terminate message from client, send terminate messages to all
 *  connected servers
 *
 *  @param connectionSocket Socket descriptor for binder for client
 *  @param messageBody      NULL
 *  @param messageBodySize  0
 *  @param connectionNumber index for connectionSocket in binderConnections
 *
 *  @return Execution result
 */
int binderDealWithTerminateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize, int connectionNumber) {
    for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
        // If this is socket for client, continue
        if (i == connectionNumber) continue;
        int eachServerSocket = binderConnections[i];
        if (eachServerSocket == 0) continue;
        // For each serverSocket, send out Terminate message
        free(messageBody);
        BYTE* messageBody = NULL;
        
        // Prepare first 8 bytes: Length(4 bytes) + Type(4 bytes)
        uint32_t messageLength = 0;
        uint32_t messageType = TERMINATE;
        uint32_t messageLength_network = htonl(messageLength);
        uint32_t messageType_network = htonl(messageType);
        
        // Send message length (4 bytes)
        ssize_t operationResult = -1;
        operationResult = send(eachServerSocket, &messageLength_network, sizeof(uint32_t), 0);
        if (operationResult != sizeof(uint32_t)) {
            perror("BINDER ERROR: TERMINATE Send message to servers length error\n");
            return -20; // ERROR -20
        }
//        printf("Binder sends termination message length succeed: %zd\n", operationResult);
        
        // Send message type (4 bytes)
        operationResult = -1;
        operationResult = send(eachServerSocket, &messageType_network, sizeof(uint32_t), 0);
        if (operationResult != sizeof(uint32_t)) {
            perror("BINDER ERROR: TERMINATE Send message to servers type error\n");
            return -21; // ERROR -21
        }
//        printf("Binder sends termination message type succeed: %zd\n", operationResult);
        
        // Send message body (varied bytes)
        operationResult = -1;
        operationResult = send(eachServerSocket, &messageBody, messageLength, 0);
        if (operationResult != messageLength) {
            perror("BINDER ERROR: TERMINATE Send message to servers body error\n");
            return -22; // ERROR -22
        }
//        printf("Binder sends termination message body succeed: %zd\n", operationResult);
    }
    return 0;
}