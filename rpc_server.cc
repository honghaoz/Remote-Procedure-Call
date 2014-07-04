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
//using namespace std;

/******************* Helper Functions ****************
 *
 *  Description: Some handy functions
 *
 ****************************************************/

/**
 *  Convert unsigned 32 bit integer to a bit string
 *
 *  @param x unsigned 32 bit integer
 *
 *  @return string representing in bit
 */
const char *u32ToBit(uint32_t x)
{
    static char b[36]; // includes 32 bits, 3 spaces and 1 trailing zero
    b[0] = '\0';
    
    u_int32_t z;
    int i = 1;
    for (z = 0b10000000000000000000000000000000; z > 0; z >>= 1, i++)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
        if (i % 8 == 0) strcat(b, " "); // seperate 8 bits by a space
    }
    
    return b;
}

void printBitRepresentation(BYTE *x, int byteNumber) {
    char bits[byteNumber * 4 + byteNumber / 4];
    bits[0] = '\0';
    //    printf("%d, %d\n", sizeof(ccc), sizeof(bits));
    int i = 0;
    while (byteNumber > 0) {
        if (byteNumber > 4) {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, 4);
            printf("%s\n", u32ToBit(xx));
            for (z = 0b10000000000000000000000000000000; z > 0; z >>= 1)
            {
                strcat(bits, ((xx & z) == z) ? "1" : "0");
            }
            i++;
            byteNumber -= 4;
            strcat(bits, " ");
//            printf("'%s'\n", bits);
        }
        else {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, byteNumber);
            printf("%s\n", u32ToBit(xx));
            for (z = pow(2, byteNumber * 8 - 1); z > 0; z >>= 1)
            {
                strcat(bits, ((xx & z) == z) ? "1" : "0");
            }
            byteNumber -= byteNumber;
//            printf("'%s'\n", bits);
        }
    }
//    printf("%s\n", bits);
}

uint32_t argTypesLength(int *argTypes) {
    uint32_t i = 0;
    while (argTypes[i] != 0) {
        i++;
    }
    return i + 1;
}

/******************* Server Functions ****************
 *
 *  Description: Server related functions
 *
 ****************************************************/

#define MAX_NUMBER_OF_CLIENTS 10
// Socket descriptor of server for clients
int serverForClientSocket; // server Listening socket
int serverConnections[MAX_NUMBER_OF_CLIENTS]; // Array of connected clients

fd_set serverSocketsFD; // Socket file descriptors that we want to wake up for
int serverHighestSocket; // Highest # of socket file descriptor

void serverBuildConnectionList();
int serverReadSockets();
int serverHandleNewConnection();
int serverDealWithData(int connectionNumber);

// Socket descriptor of server to binder
int serverToBinderSocket;

// Server identifier and port number
char ipv4Address[INET_ADDRSTRLEN];
int portNumber;

std::map<std::pair<char *, int *>, skeleton> serverProcedureToSkeleton;

/**
 *  rpcInit()
 *
 *  1,  Set up server listen sockets for clients
 *  2,  Create socket connection with Binder
 *
 *  @return result of rpcInit()
 */
int rpcInit() {
    std::cout << "rpcInit()" << std::endl;
    
    // 1, Set up server listen sockets for clients
    
    // Get IPv4 address for this host
    struct addrinfo hints, *res, *p;
    int status;
    
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
            inet_ntop(p->ai_family, addr, ipv4Address, sizeof ipv4Address);
            printf("SERVER_ADDRESS %s\n", ipv4Address);
            break;
        }
    }
    freeaddrinfo(res); // Free the linked list
    
    // Create socket
    serverForClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverForClientSocket == -1) {
        perror("Could not create socket\n");
        return -1;
    }
    
    int addr_size;
    setsockopt(serverForClientSocket, SOL_SOCKET, SO_REUSEADDR, &addr_size, sizeof(addr_size)); // So that we can re-bind to it without TIME_WAIT problems

    // Prepare the sockaddr_in structure
    struct sockaddr_in clientForServer;
    memset(&clientForServer, 0, sizeof(struct sockaddr_in));
    clientForServer.sin_family = AF_INET;
    clientForServer.sin_addr.s_addr = htonl(INADDR_ANY);
    clientForServer.sin_port = htons(0);
    
    // Bind
    int bindResult = bind(serverForClientSocket, (struct sockaddr *)&clientForServer, sizeof(struct sockaddr_in));
    if(bindResult == -1) {
        perror("Bind failed");
        return -1;
    }

    // Listen
    listen(serverForClientSocket , MAX_NUMBER_OF_CLIENTS);

    // Print out port number
    socklen_t addressLength = sizeof(clientForServer);
    if (getsockname(serverForClientSocket, (struct sockaddr*)&clientForServer, &addressLength) == -1) {
        perror("Get port error");
        return -1;
    }
    printf("SERVER_PORT %d\n", ntohs(clientForServer.sin_port));
    portNumber = ntohs(clientForServer.sin_port);
    
    // Since there is only one sock, this is the highest
    serverHighestSocket = serverForClientSocket;
    // Clear the clients
    memset((char *) &serverConnections, 0, sizeof(serverConnections));
    
    // 2, Create socket connection with Binder
    serverToBinderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverToBinderSocket == -1) {
        perror("Server to binder: Could not create socket\n");
        return -1;
    }

    struct sockaddr_in serverToBinder;
    // Set address
#warning Need to change to dynamic address
    serverToBinder.sin_addr.s_addr = inet_addr("127.0.0.1");//getenv("BINDER_ADDRESS"));
    serverToBinder.sin_family = AF_INET;
    serverToBinder.sin_port = htons(8888);//htons(atoi(getenv("BINDER_PORT")));
    

    // Connect
    if (connect(serverToBinderSocket, (struct sockaddr *)&serverToBinder, sizeof(struct sockaddr_in)) == -1) {
        perror("Server to binder: Connect failed. Error");
        return -1;
    }
    
    return 0;
}

/**
 *  rpcRegister(char* name, int* argTypes, skeleton f)
 *
 *  1,  Server add map entry: (name, argTypes) -> f
 *  2,  Send procedure to binder and register server procedure
 *      Message format is Length(4 bytes) + Type(4 bytes) + Message
 *      Message type is MSG_SERVER_BINDER_REGISTER
 *      Message is REGISTER, server_identifier, port, name, argTypes
 *  3,  Receive registration response from binder
 *
 *  @return result of rpcRegister()
 */
int rpcRegister(char* name, int* argTypes, skeleton f) {
    std::cout << "rpcRegister(" << name << ")" << std::endl;
    
    //1,  Server add map entry: (name, argTypes) -> f
    std::pair<char *, int *> theProcedureSignature (name, argTypes);
    serverProcedureToSkeleton[theProcedureSignature] = f;
    
    //2,  Send procedure to binder and register server procedure
    
    // Prepare message content
    uint32_t sizeOfIp = (uint32_t)strlen(ipv4Address) + 1;
    uint32_t sizeOfPort = sizeof(portNumber);
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t totalSize = sizeOfIp + sizeOfPort + sizeOfName + sizeOfArgTypes + 4;
//    printf("%d + 1 + %d + 1 + %d + 1 + %d + 1 = %d\n", sizeOfIp, sizeOfPort, sizeOfName, sizeOfArgTypes, totalSize);
    
    BYTE messageBody[totalSize];
    char seperator = ',';
    memcpy(messageBody, ipv4Address, sizeOfIp); // [ip]
    memcpy(messageBody + sizeOfIp, &seperator, 1); // [ip,]
    memcpy(messageBody + sizeOfIp + 1, &portNumber, sizeOfPort); // [ip,portnum]
    memcpy(messageBody + sizeOfIp + 1 + sizeOfPort, &seperator, 1); // [ip,portnum,]
    memcpy(messageBody + sizeOfIp + 1 + sizeOfPort + 1, name, sizeOfName); // [ip,portnum,name]
    memcpy(messageBody + sizeOfIp + 1 + sizeOfPort + 1 + sizeOfName, &seperator, 1); // [ip,portnum,name,]
    memcpy(messageBody + sizeOfIp + 1 + sizeOfPort + 1 + sizeOfName + 1, argTypes, sizeOfArgTypes); // [ip,portnum,name,argTypes]
    memcpy(messageBody + sizeOfIp + 1 + sizeOfPort + 1 + sizeOfName + 1 + sizeOfArgTypes, &seperator, 1); // [ip,portnum,name,argTypes]
    
    // Prepare first 8 bytes: Length(4 bytes) + Type(4 bytes)
    uint32_t messageLength = totalSize;
    uint32_t messageType = REGISTER;
    uint32_t messageLength_network = htonl(messageLength);
    uint32_t messageType_network = htonl(messageType);
    
    // Send message length (4 bytes)
    ssize_t operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageLength_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server registion to binder: Send message length failed\n");
        return -1;
    }
    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server registion to binder: Send message type failed\n");
        return -1;
    }
    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Server registion to binder: Send message body failed\n");
        return -1;
    }
    printf("Send message body succeed: %zd\n", operationResult);
//    for (int i = 0; i < messageLength; i++) {
//        printf("%c ", messageBody[i]);
//    }
//    printf("\n");
//
//    printf("IP: %s\n", ipv4Address);
//    printf("Port: %d\n", portNumber);
//    printf("Name: %s\n", name);
//    printf("ArgTypes: ");
//    for (int i = 0; i < argTypesLength(argTypes); i++) {
//        printf("%ud ", argTypes[i]);
//    }
//    printf("\n");
    
    // 3,  Receive registration response from binder
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    ssize_t receivedSize = -1;
    receivedSize = recv(serverToBinderSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Server receive binder response failed\n");
        return -1;
    }
    responseType = ntohl(responseType_network);
    if (responseType == REGISTER_FAILURE) {
        perror("Binder response: REGISTER_FAILURE\n");
        return -1;
    } else if (responseType == REGISTER_SUCCESS) {
        printf("Binder response: REGISTER_SUCCESS\n");
        return 0;
    } else {
        return 0;
    }
}

/**
 *  rpcExecute()
 *  
 *  1, Handle client calls
 *
 *  @return result of rpcExecute()
 */
int rpcExecute() {
    std::cout << "rpcExecute()" << std::endl;
//    sleep(2);
    int numberOfReadSockets; // Number of sockets ready for reading
    // Server loop
    while (1) {
        serverBuildConnectionList();
        //        timeout.tv_sec = 1;
        //        timeout.tv_usec = 0;
        numberOfReadSockets = select(serverHighestSocket + 1, &serverSocketsFD, NULL, NULL, NULL);
        if (numberOfReadSockets < 0) {
            perror("Select error");
            return -1;
        }
        if (numberOfReadSockets > 0) {
            if (serverReadSockets() < 0) {
                return -1;
            }
        }
    }
    
    close(serverForClientSocket);
    
    return 0;
}

void serverBuildConnectionList() {
    // Clears out the fd_set called socks
	FD_ZERO(&serverSocketsFD);
	
    // Adds listening sock to socks
	FD_SET(serverForClientSocket, &serverSocketsFD);
	
    // Add the new connection sock to socks
    for (int i = 0; i < MAX_NUMBER_OF_CLIENTS; i++) {
        if (serverConnections[i] != 0) {
            FD_SET(serverConnections[i], &serverSocketsFD);
            if (serverConnections[i] > serverHighestSocket) {
                serverHighestSocket = serverConnections[i];
            }
        }
    }
}
int serverReadSockets() {
    // If listening sock is woked up, there is a new client come in
    if (FD_ISSET(serverForClientSocket, &serverSocketsFD)) {
        if(serverHandleNewConnection() < 0) {
            return -1;
        }
    }
    
    // Check whether there is new data come in
    for (int i = 0; i < MAX_NUMBER_OF_CLIENTS; i++) {
        if (FD_ISSET(serverConnections[i], &serverSocketsFD)) {
            if (serverDealWithData(i) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

int serverHandleNewConnection() {
    int newSock;
    struct sockaddr_in client;
    int addr_size = sizeof(struct sockaddr_in);
    
    // Try to accept a new connection
    newSock = accept(serverForClientSocket, (struct sockaddr *)&client, (socklen_t *)&addr_size);
    if (newSock < 0) {
        perror("Accept failed");
        return -1;
    }
    // Add this new client to clients
    for (int i = 0; (i < MAX_NUMBER_OF_CLIENTS) && (i != -1); i++) {
        if (serverConnections[i] == 0) {
            printf("\nConnection accepted: FD=%d; Slot=%d\n", newSock, i);
            serverConnections[i] = newSock;
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

int serverDealWithData(int connectionNumber) {
    // Get the socket descriptor
    int connectionSocket = serverConnections[connectionNumber];
    
    // Dispatch a new thread to handle procedure execution
    
    return 0;
}

