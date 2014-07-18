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
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
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
        perror("Could not create socket\n");
        return -1;
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
        perror("Bind failed");
        return -1;
    }
    
    // Listen
    listen(binderListenSocket , MAX_NUMBER_OF_CONNECTIONS);
    
    // Print out port number
    socklen_t addressLength = sizeof(binder);
    if (getsockname(binderListenSocket, (struct sockaddr*)&binder, &addressLength) == -1) {
        perror("Get port error");
        return -1;
    }
    printf("BINDER_PORT %d\n", ntohs(binder.sin_port));
    
    // Since there is only one sock, this is the highest
    binderHighestSocket = binderListenSocket;
    // Clear the clients
    memset((char *) &binderConnections, 0, sizeof(binderConnections));
    return 0;
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
            perror("Select error");
            return -1;
        }
        if (numberOfReadSockets > 0) {
            if (binderReadSockets() == BINDER_TERMINATE) {
                //return -1;
//                printf("Binder will terminate\n");
                break;
            }
        }
    }
    
    close(binderListenSocket);
    printf("Binder terminated!\n");
    return 0;
}
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
int binderReadSockets() {
    // If listening sock is woked up, there is a new client come in
    if (FD_ISSET(binderListenSocket, &binderSocketsFD)) {
        if(binderHandleNewConnection() < 0) {
            return -1;
        }
    }
    
    // Check whether there is new data come in
    for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
        if (FD_ISSET(binderConnections[i], &binderSocketsFD)) {
            int resultOfBinderDealWithData = binderDealWithData(i);
            if (resultOfBinderDealWithData < 0) {
                return -1;
            } else if (resultOfBinderDealWithData == BINDER_TERMINATE) {
                return BINDER_TERMINATE;
            }
        }
    }
    return 0;
}
int binderHandleNewConnection() {
    int newSock;
    struct sockaddr_in client;
    int addr_size = sizeof(struct sockaddr_in);
    
    // Try to accept a new connection
    newSock = accept(binderListenSocket, (struct sockaddr *)&client, (socklen_t *)&addr_size);
    if (newSock < 0) {
        perror("Accept failed");
        return -1;
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
        perror("No room left for new client.\n");
        close(newSock);
        return -1;
    }
    
    return 0;
}

// Server send back response
int binderResponse(int connectionSocket, messageType responseType, uint32_t errorCode) {
    // Send response message to server
    uint32_t responseType_network = htonl(responseType);
    uint32_t responseErrorCode_network = htonl(errorCode);
    
    // Send response type
    ssize_t operationResult = -1;
    operationResult = send(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Binder sends response type failed\n");
        return -1;
    }
    
    // Send response error code
    operationResult = -1;
    operationResult = send(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Binder sends response errorCode failed\n");
        return -1;
    }
//    printf("Binder responseType: %d, errorCode: %d\n", responseType, errorCode);
    
    return 0;
}

// Binder deal with three different message: Register, Locate and Terminate
int binderDealWithRegisterMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithCachedLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize);
int binderDealWithTerminateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize, int connectionNumber);

// Receive message length, message type and allocate message body
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
                return 0;
            }
        }
        return BINDER_TERMINATE;
    }
    else if (receivedSize != sizeof(uint32_t)) {
        perror("Binder received wrong length of message length\n");
        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -1;
//        return 0;
    }
    else { // Receive message length correctly
//        printf("Received length of message length: %zd\n", receivedSize);
        messageLength = ntohl(messageLength_network);
    }
    
    // Receive message type
    receivedSize = -1;
    receivedSize = recv(connectionSocket, &messageType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Binder received wrong length of message type\n");
//        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -1;
//        return 0;
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
        perror("Binder received wrong length of message body\n");
        binderResponse(connectionSocket, REGISTER_FAILURE, 1);
        return -1;
//        return 0;
    }
//    printf("Received length of message body: %zd\n", receivedSize);
    
    switch (messageType) {
        case REGISTER: {
            printf("\n******************* NEW REGISTER *******************\n");
            return binderDealWithRegisterMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        case LOC_REQUEST: {
            printf("\n******************* NEW LOCATE *******************\n");
            return binderDealWithLocateMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        case TERMINATE: {
            printf("\n******************* NEW TERMINATE *******************\n");
            return binderDealWithTerminateMessage(connectionSocket, messageBody, receivedSize, connectionNumber);
            break;
        }
        case LOC_CACHED_REQUEST: {
            printf("\n***************** NEW CACHED LOCATE *******************\n");
            return binderDealWithCachedLocateMessage(connectionSocket, messageBody, receivedSize);
            break;
        }
        default:
            perror("Binder received unknown message type\n");
            return -1;
//            return 0;
            break;
    }
    free(messageBody);
    
    return 0;
}

int binderDealWithRegisterMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize) {
    // Message body: [ip,portnum,name,argTypes,]
    char *ipv4Address = NULL;
    int portNumber = 0;
    char *name = NULL;
    int *argTypes = NULL;
    
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    enum messageType responseType = REGISTER_SUCCESS;
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < messageBodySize && responseType == REGISTER_SUCCESS; i++) {
        if (messageBody[i] == ',') {
            switch (messageCount) {
                case 0: {
                    uint32_t sizeOfIp = i - (lastSeperatorIndex + 1);
                    if (sizeOfIp > 16) {
                        // Size is too large
                        responseType = REGISTER_FAILURE;
                    }
//                    printf("%d\n", sizeOfIp);
                    ipv4Address = (char*)malloc(sizeof(char) * sizeOfIp);
                    memcpy(ipv4Address, messageBody + lastSeperatorIndex + 1, sizeOfIp);
                    break;
                }
                case 1: {
                    uint32_t sizeOfPort = i - (lastSeperatorIndex + 1);
                    if (sizeOfPort != sizeof(portNumber)) {
                        perror("port size error\n");
                        responseType = REGISTER_FAILURE;
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
                    perror("Message Body Error\n");
                    responseType = REGISTER_FAILURE;
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    // Process completed
    
    printf("Registered: IP: %s, Port: %d, Name: %s\n", ipv4Address, portNumber, name);
//    printf("ArgTypes: ");
//    for (int i = 0; i < argTypesLength(argTypes); i++) {
//        printf("%ud ", argTypes[i]);
//    }
//    printf("\n");
    
    // If message is correct, register, else free
    if (responseType == REGISTER_SUCCESS) {
        // Store this procedure in binder's data store
        P_NAME_TYPES procedureNameTypes(name, argTypes);
        P_NAME_TYPES_SOCKET procedureKey(procedureNameTypes, connectionSocket);
        P_IP_PORT ID(ipv4Address, portNumber);
        binderProcedureToID.insert(procedureKey, ID);
    } else  {
        free(ipv4Address);
        free(name);
        free(argTypes);
    }
    binderResponse(connectionSocket, responseType, 1);
    if (responseType == REGISTER_SUCCESS) {
        return 0;
    } else {
        return -1;
//        return 0;
    }
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
    for (int i = 0; i < messageBodySize /*&& responseType == LOC_SUCCESS*/; i++) {
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
                    perror("Message Body Error\n");
//                    responseType = LOC_FAILURE;
                    binderResponse(connectionSocket, LOC_FAILURE, -1);
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    return -1;
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
//    printf("Locate: %s\n", name);
//    printf("Locate ArgTypes: ");
//    for (int i = 0; i < argTypesLength(argTypes); i++) {
//        printf("%ud ", argTypes[i]);
//    }
//    printf("\n");
    
    P_NAME_TYPES queryKey(name, argTypes);
    P_IP_PORT *queryResult = binderProcedureToID.findIp_client(queryKey);
    if (queryResult == NULL) {
        perror("Procedure Not found");
        binderResponse(connectionSocket, LOC_FAILURE, -1);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
//        return 0;
        return -1;
    }
    ipv4Address = queryResult->first;
    portNumber = queryResult->second;
    printf("Locate: %s, found IP: %s, Port: %d\n", name, ipv4Address, portNumber);
    
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
        perror("Binder to server: Send ip+portnum failed\n");
//        return 0;
        return -1;
    }
//    printf("Response located IP: %s\n", ipv4Address);
//    printf("Response located Port: %d\n", portNumber);
    
    if (name != NULL) free(name);
    if (argTypes != NULL) free(argTypes);
    return 0;
}

int binderDealWithCachedLocateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize) {
    // Message body: [name,argTypes,]
    char *name = NULL;
    int *argTypes = NULL;
#warning meeeeee
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
                    perror("Message Body Error\n");
                    //                    responseType = LOC_FAILURE;
                    binderResponse(connectionSocket, LOC_FAILURE, -1);
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    return -1;
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
        perror("Procedure Not found");
        binderResponse(connectionSocket, LOC_CACHED_FAILURE, -1);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        //        return 0;
        return -1;
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
        perror("Binder cached call response to client: Send message length failed\n");
        return -1;
    }
    printf("Binder cached call send message length succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(connectionSocket, &messageBodyResponse, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Binder cached call response to binder: Send message body failed\n");
        return -1;
    }
    printf("Binder cached call send message body succeed: %zd\n", operationResult);
    return 0;
}


int binderDealWithTerminateMessage(int connectionSocket, BYTE *messageBody, ssize_t messageBodySize, int connectionNumber) {
    for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
//        printf("Check %dth socket\n", i);
        // If this is socket for client, continue
        if (i == connectionNumber) continue;
        int eachServerSocket = binderConnections[i];
        if (eachServerSocket == 0) continue;
        // For each serverSocket, send out Terminate message
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
            perror("Binder termination message to server: Send message length failed\n");
            return -1;
        }
//        printf("Binder sends termination message length succeed: %zd\n", operationResult);
        
        // Send message type (4 bytes)
        operationResult = -1;
        operationResult = send(eachServerSocket, &messageType_network, sizeof(uint32_t), 0);
        if (operationResult != sizeof(uint32_t)) {
            perror("Binder termination message to server: Send message type failed\n");
            return -1;
        }
//        printf("Binder sends termination message type succeed: %zd\n", operationResult);
        
        // Send message body (varied bytes)
        operationResult = -1;
        operationResult = send(eachServerSocket, &messageBody, messageLength, 0);
        if (operationResult != messageLength) {
            perror("Binder termination message to server: Send message body failed\n");
            return -1;
        }
//        printf("Binder sends termination message body succeed: %zd\n", operationResult);
    }
    return 0;
}