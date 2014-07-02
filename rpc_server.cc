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

void printBitRepresentation(BIT *x, int byteNumber) {
    char ccc[byteNumber * 4 + byteNumber / 4];
    ccc[0] = '\0';
    //    printf("%d, %d\n", sizeof(ccc), sizeof(bits));
    int i = 0;
    while (byteNumber > 0) {
        if (byteNumber > 4) {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, 4);
            for (z = 0b10000000000000000000000000000000; z > 0; z >>= 1)
            {
                strcat(ccc, ((xx & z) == z) ? "1" : "0");
            }
            i++;
            byteNumber -= 4;
            strcat(ccc, " ");
        }
        else {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, byteNumber);
            for (z = pow(2, byteNumber * 8 - 1); z > 0; z >>= 1)
            {
                strcat(ccc, ((xx & z) == z) ? "1" : "0");
            }
            byteNumber -= byteNumber;
        }
    }
    printf("%s\n", ccc);
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
int serverForClientSocket;
// Socket descriptor of server to binder
int serverToBinderSocket;

// Server identifier and port number
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
 *  1,  Send procedure to binder and register server procedure
 *  2,  Message format is Length(4 bytes) + Type(4 bytes) + Message
 *      Message type is MSG_SERVER_BINDER_REGISTER
 *      Message is REGISTER, server_identifier, port, name, argTypes
 *  3,  Server matches name and argTypes with skeleton
 *
 *  @return result of rpcRegister()
 */

int rpcRegister(char* name, int* argTypes, skeleton f) {
    std::cout << "rpcRegister(" << name << ")" << std::endl;
    
    // Prepare message content
    uint32_t sizeOfIp = (uint32_t)strlen(ipv4Address) + 1;
    uint32_t sizeOfport = sizeof(portNumber);
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t totalSize = sizeOfIp + sizeOfport + sizeOfName + sizeOfArgTypes + 3;
    printf("%d + %d + %d + %d = %d\n", sizeOfIp, sizeOfport, sizeOfName, sizeOfArgTypes, totalSize);
    
    BIT message[totalSize];
    char seperator = ',';
    memcpy(message, ipv4Address, sizeOfIp); // [ip]
    memcpy(message + sizeOfIp, &seperator, 1); // [ip,]
    memcpy(message + sizeOfIp + 1, &portNumber, sizeOfport); // [ip,portnum]
    memcpy(message + sizeOfIp + 1 + sizeOfport, &seperator, 1); // [ip,portnum,]
    memcpy(message + sizeOfIp + 1 + sizeOfport + 1, name, sizeOfName); // [ip,portnum,name]
    memcpy(message + sizeOfIp + 1 + sizeOfport + 1 + sizeOfName, &seperator, 1); // [ip,portnum,name,]
    memcpy(message + sizeOfIp + 1 + sizeOfport + 1 + sizeOfName + 1, argTypes, sizeOfArgTypes); // [ip,portnum,name,argTypes]
    
    // Prepare first 8 bytes: Length(4 bytes) + Type(4 bytes)
    uint32_t messageLength = totalSize;
    uint32_t messageType = MSG_SERVER_BINDER_REGISTER;
    
    return 0;
}

int rpcExecute() {
    std::cout << "rpcExecute()" << std::endl;
    return 0;
}

