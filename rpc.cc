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

/******************* Server Functions ****************
 *
 *  Description: Server related functions
 *
 ****************************************************/

#define MAX_NUMBER_OF_CLIENTS 10
int serverForClientSocket;
int serverToBinderSocket;

char ipv4Address[INET_ADDRSTRLEN];
int portNumber;

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
    
    // Does need to handle multi clients???
    
    // 2, Create socket connection with Binder
    serverToBinderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverToBinderSocket == -1) {
        perror("Server to binder: Could not create socket\n");
        return -1;
    }

    struct sockaddr_in serverToBinder;
    // Set address
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
 *  1,  Send procedure to binder and register server procedure
 *  2,  Message type is MSG_SERVER_BINDER_REGISTER
 *      Message is REGISTER, server_identifier, port, name, argTypes
 *
 *  @return result of rpcRegister()
 */

int rpcRegister(char* name, int* argTypes, skeleton f) {
    std::cout << "rpcRegister(" << name << ")" << std::endl;
    
    return 0;
}

int rpcExecute() {
    std::cout << "rpcExecute()" << std::endl;
    return 0;
}

/******************* Client Functions ****************
 *
 *  Description: Client related functions
 *
 ****************************************************/

int rpcCall(char* name, int* argTypes, void** args) {
    std::cout << "rpcCall(" << name << ")" << std::endl;
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

void binderBuildConnectionList();
int binderReadSockets();
int binderHandleNewConnection();
int binderDealWithData(int connectionNumber);

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
            if (binderReadSockets() < 0) {
                return -1;
            }
        }
    }
    
    close(binderListenSocket);
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
            if (binderDealWithData(i) < 0) {
                return -1;
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
            printf("\nConnection accepted: FD=%d; Slot=%d\n", newSock, i);
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

int binderDealWithData(int connectionNumber) {
    // Get the socket descriptor
    int connectionSocket = binderConnections[connectionNumber];
    long receive_size;
    
    // Prepare for string length
    uint32_t network_byte_order;
    uint32_t string_length;
    
    // Receive a message from client
    receive_size = recv(connectionSocket, &network_byte_order, sizeof(uint32_t), 0);
    
    // Receive successfully
    if (receive_size > 0){
        // Receive size must be same as sizeof(uint32_t) = 4
        if (receive_size != sizeof(uint32_t)) {
            printf("string length error\n");
        }
        //        printf("    *receive_size:%lu -   ", receive_size);
        // Get string length
        string_length = ntohl(network_byte_order);
        
        // Prepare memeory for string body
        char *client_message = (char *)malloc(sizeof(char) * string_length);
        // Revice string
        receive_size = recv(connectionSocket, client_message, string_length, 0);
        
        //        printf("stringlength:%u - received_size: %zd\n", string_length, receive_size);
        if (receive_size == string_length) {
            //            struct timeval now;
            //            gettimeofday(&now, NULL);
            //            printf("%s - %ld.%d\n", client_message, now.tv_sec, now.tv_usec);
            printf("%s\n", client_message);
//            process_to_title_case(client_message);
            // Send string length
            send(connectionSocket, &network_byte_order, sizeof(uint32_t), 0);
            // Send string
            if(send(connectionSocket, client_message, receive_size, 0) == -1) {
                //                printf("Send failed\n");
            }
        } else {
            printf("string error: ");
        }
        free(client_message);
    }
    else if (receive_size < 0) {
        perror("Receive failed");
        return -1;
    }
    else {
        // printf("\nConnection lost: FD=%d;  Slot=%d\n", client_sock, client_number);
        close(connectionSocket);
        
        // Set this place to be available
        binderConnections[connectionNumber] = 0;
    }
    return 0;
}