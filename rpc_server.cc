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
#include <pthread.h>
#include <assert.h>
#include "rpc.h"
#include "pmap.h"
//using namespace std;

/******************* Server  ************************
 *
 *  Description: Server related variables and functions
 *
 ****************************************************/

#define MAX_NUMBER_OF_CLIENTS 100
// Socket descriptor of server for clients
int serverForClientSocket; // server Listening socket
int serverConnections[MAX_NUMBER_OF_CLIENTS]; // Array of connected clients
fd_set serverSocketsFD; // Socket file descriptors that we want to wake up for
int serverHighestSocket; // Highest # of socket file descriptor

// Socket descriptor of server to binder
int serverToBinderSocket;
// Socket descriptor

// Server identifier and port number
char ipv4Address[INET_ADDRSTRLEN];
uint32_t portNumber;
// Server registered procedures
//std::map<std::pair<char *, int *>, skeleton> serverProcedureToSkeleton;
pmap serverProcedureToSkeleton;

pthread_mutex_t mutex;

struct threadArgs {
    BYTE *messageBody;
    int connectionSocket;
    long receivedSize;
    int messageLength;
};

#define SERVER_TERMINATE 999


#pragma mark - rpcInit()
/************************* rpcInit() *************************
 *
 *  1,  Set up server listen sockets for clients
 *  2,  Create socket connection with Binder
 *
 *************************************************************/
int serverForClientsInit();
int serverToBinderInit();

int rpcInit()
{
    std::cout << "rpcInit()" << std::endl;
    // 1, Set up server listen sockets for clients
    int status = serverForClientsInit();
    if (status < 0) {
        return -1;
    }

    // 2, Create socket connection with Binder
    status = -1;
    status = serverToBinderInit();
    if (status < 0) {
        return -1;
    }

    return 0;
}

// Init socket for clients and listen it
int serverForClientsInit() {
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
    return 0;
}

// Init socket to binder and connect it
int serverToBinderInit() {
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
    
    // Add serverToBinder socket to connectionList
    if (serverToBinderSocket > serverHighestSocket) {
        serverHighestSocket = serverToBinderSocket;
    }
    serverConnections[0] = serverToBinderSocket;
    
    return 0;
}








#pragma mark - rpcRegister()
/******** rpcRegister(char* name, int* argTypes, skeleton f) **********
 *
 *  1,  Server add map entry: (name, argTypes) -> f
 *  2,  Send procedure to binder and register server procedure
 *      Message format is Length(4 bytes) + Type(4 bytes) + Message
 *      Message type is REGISTER
 *      Message is [server_identifier, port, name, argTypes]
 *  3,  Receive registration response from binder
 *
 **********************************************************************/
int serverHandleResponse(int connectionSocket);

int rpcRegister(char* name, int* argTypes, skeleton f) {
    std::cout << "rpcRegister(" << name << ")" << std::endl;
    
    //1,  Server add map entry: (name, argTypes) -> f
    P_NAME_TYPES theProcedureSignature (name, argTypes);
    serverProcedureToSkeleton.insert(theProcedureSignature, f);
    
    //2,  Send procedure to binder and register server procedure
    
    // Prepare message content
    uint32_t sizeOfIp = 16;//(uint32_t)strlen(ipv4Address) + 1;
    uint32_t sizeOfPort = sizeof(portNumber);
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t totalSize = sizeOfIp + sizeOfPort + sizeOfName + sizeOfArgTypes + 4;
//    printf("%d + 1 + %d + 1 + %d + 1 + %d + 1 = %d\n", sizeOfIp, sizeOfPort, sizeOfName, sizeOfArgTypes, totalSize);
    
    BYTE messageBody[totalSize];
    char seperator = ',';
    uint32_t sizeOfSeperator = sizeof(seperator);
    int offset = 0;
    memcpy(messageBody + offset, ipv4Address, sizeOfIp); // [ip]
    offset += sizeOfIp;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,]
    offset += sizeOfSeperator;
    memcpy(messageBody + offset, &portNumber, sizeOfPort); // [ip,portnum]
    offset += sizeOfPort;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,portnum,]
    offset += sizeOfSeperator;
    memcpy(messageBody + offset, name, sizeOfName); // [ip,portnum,name]
    offset += sizeOfName;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,portnum,name,]
    offset += sizeOfSeperator;
    memcpy(messageBody + offset, argTypes, sizeOfArgTypes); // [ip,portnum,name,argTypes]
    offset += sizeOfArgTypes;
    memcpy(messageBody + offset, &seperator, sizeOfSeperator); // [ip,portnum,name,argTypes,]
    
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
//    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server registion to binder: Send message type failed\n");
        return -1;
    }
//    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        perror("Server registion to binder: Send message body failed\n");
        return -1;
    }
//    printf("Send message body succeed: %zd\n", operationResult);
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
    
//    printf("\n");
    
    // 3,  Receive registration response from binder
    return serverHandleResponse(serverToBinderSocket);
}

int serverHandleResponse(int connectionSocket) {
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    uint32_t responseErrorCode_network = 0;
    int responseErrorCode = 0;
    
    // Receive response type
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Server receive binder response type failed\n");
        return -1;
    }
    receivedSize = -1;
    
    // Receive response error code
    receivedSize = recv(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Server receive binder response errorCode failed\n");
        return -1;
    }
    
    responseType = ntohl(responseType_network);
    responseErrorCode = ntohl(responseErrorCode_network);
    
    if (responseType == REGISTER_FAILURE) {
        //perror("Binder response: REGISTER_FAILURE Error Code: %d\n");
        std::cerr << "Binder response: REGISTER_FAILURE Error Code: " << responseErrorCode << std::endl;
        return responseErrorCode;
    } else if (responseType == REGISTER_SUCCESS) {
//        printf("Binder response: REGISTER_SUCCESS\n");
        return 0;
    } else {
        return 0;
    }
}






#pragma mark - rpcExecute()
/************************ rpcExecute() **************************
 *  
 *  1, Handle client calls. Receive data from clients
 *  2, Call skeleton
 *  3, Send back the results
 *
 ****************************************************************/

// Server for clients functions
void serverBuildConnectionList();
int serverReadSockets();
int serverHandleNewConnection();
int serverDealWithData(int connectionNumber);
void *serverHandleNewExecution(void *t);

int rpcExecute() {
    std::cout << "rpcExecute()" << std::endl;
    int numberOfReadSockets; // Number of sockets ready for reading
    // Server loop
    while (1) {
        serverBuildConnectionList();
        numberOfReadSockets = select(serverHighestSocket + 1, &serverSocketsFD, NULL, NULL, NULL);
        if (numberOfReadSockets < 0) {
            perror("Select error");
            return -1;
        }
        if (numberOfReadSockets > 0) {
            if (serverReadSockets() == SERVER_TERMINATE) {
                //return -1;
//                printf("Server will terminate\n");
                break;
            }
        }
    }
    close(serverForClientSocket);
    printf("Server terminated\n");
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
            int resultOfServerDealWithData = serverDealWithData(i);
            if (resultOfServerDealWithData < 0) {
                return -1;
            } else if (resultOfServerDealWithData == SERVER_TERMINATE) {
                return SERVER_TERMINATE;
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
//            printf("\nConnection accepted: FD=%d; Slot=%d\n", newSock, i);
            pthread_mutex_lock(&mutex);
            serverConnections[i] = newSock;
            pthread_mutex_unlock(&mutex);
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
int serverResponse(int connectionSocket, messageType responseType, uint32_t errorCode) {
    // Send response message to server
    uint32_t responseType_network = htonl(responseType);
    uint32_t responseErrorCode_network = htonl(errorCode);
    
    // Send response type
    ssize_t operationResult = -1;
    operationResult = send(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server sends response failed\n");
        return -1;
    }
    
    // Send response error code
    operationResult = -1;
    operationResult = send(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        perror("Server sends response errorCode failed\n");
        return -1;
    }
//    printf("Server responseType: %d, errorCode: %d\n", responseType, errorCode);
    return 0;
}

int serverDealWithData(int connectionNumber) {
    // 1, Receive data from client
    
    // Get the socket descriptor
    int connectionSocket = serverConnections[connectionNumber];
    // Expected Message: [MessageLength][MessageType][name,argTypes,args,]
    
    // Prepare for message length, message type
    uint32_t messageLength_network = 0;
    uint32_t messageType_network = 0;
    uint32_t messageLength = 0; // Will also be used to send back
    uint32_t messageType = 0;
    
    // Receive message length
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &messageLength_network, sizeof(uint32_t), 0);
    // Connection lost
    if (receivedSize == 0) {
//        printf("\nConnection lost: FD=%d;  Slot=%d\n", connectionSocket, connectionNumber);
        close(connectionSocket);
        
        // Set this place to be available
        pthread_mutex_lock(&mutex);
        serverConnections[connectionNumber] = 0;
        pthread_mutex_unlock(&mutex);
        return 0;
//        pthread_exit((void*) 0);
    }
    else if (receivedSize != sizeof(uint32_t)) {
        perror("Server received wrong length of message length\n");
        serverResponse(connectionSocket, EXECUTE_FAILURE, -1);
//        pthread_exit((void*) 0);;
        return 0;
    }
    else { // Receive message length correctly
//        printf("Received length of message length: %zd\n", receivedSize);
        //        serverResponse(connectionSocket, EXECUTE_FAILURE, -1);
        messageLength = ntohl(messageLength_network);
    }
    
    // Receive message type
    receivedSize = -1;
    receivedSize = recv(connectionSocket, &messageType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        perror("Server received wrong length of message type\n");
        serverResponse(connectionSocket, EXECUTE_FAILURE, -1);
//        pthread_exit((void*) 0);
        return 0;
    } else { // Receive message length correctly
//        printf("Received length of message type: %zd\n", receivedSize);
        messageType = ntohl(messageType_network);
    }
    // Server should terminate
    if (messageType == TERMINATE) {
//        printf("Server received TERMINATE message\n");
//        printf("Authenticating... ");
        if (connectionSocket == serverToBinderSocket) {
//            printf("message comes from binder. Accept\n");
            return SERVER_TERMINATE;
        }
//        printf("message doesn't come from binder. Reject\n");
    }
    
    // If type is not EXECUTE
    if (messageType != EXECUTE) {
        perror("Server received wrong message type\n");
        serverResponse(connectionSocket, EXECUTE_FAILURE, -1);
//        pthread_exit((void*) 0);
        return 0;
    }
    
    // Allocate messageBody
    BYTE *messageBody = (BYTE *)malloc(sizeof(BYTE) * messageLength);
    
    // Receive message body
    receivedSize = -1;
    receivedSize = recv(connectionSocket, messageBody, messageLength, 0);
    if (receivedSize != messageLength) {
        perror("Server received wrong length of message body\n");
//        pthread_exit((void*) 0);
        return 0;
    }
//    printf("Received length of message body: %zd\n", receivedSize);
    
    // 2, Dispatch a new thread to handle procedure execution
    threadArgs args;
    args.messageBody = messageBody;
    args.receivedSize = receivedSize;
    args.messageLength = messageLength;
    args.connectionSocket = connectionSocket;
    
    pthread_t newExecutionThread;
    int threadCreatedResult = pthread_create(&newExecutionThread, NULL, serverHandleNewExecution, (void *)&args);
    if (threadCreatedResult != 0) {
//        printf("Dispatch new execution thread failed: %d\n", threadCreatedResult);
        return -1;
    }
//    printf("Dispatch new execution thread succeed\n");
    return 0;
}

void* serverHandleNewExecution(void *t) {
//    printf("New thread start\n");
    threadArgs p_args = *(threadArgs *)t;
    BYTE *messageBody = p_args.messageBody;
    
    // Message body: [name,argTypes,argsByte,]
    char *name = NULL;
    int *argTypes = NULL;
    BYTE *argsByte = NULL; //expanded args
    void ** args = NULL;
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    enum messageType responseType = EXECUTE_SUCCESS;
    
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < p_args.receivedSize && responseType == EXECUTE_SUCCESS; i++) {
        if (messageBody[i] == ',') {
            switch (messageCount) {
                case 0: {
                    uint32_t sizeOfName = i - (lastSeperatorIndex + 1);
                    name = (char *)malloc(sizeof(char) * sizeOfName);
                    memcpy(name, messageBody + lastSeperatorIndex + 1, sizeOfName);
                    break;
                }
                case 1: {
                    uint32_t sizeOfArgTypes = i - (lastSeperatorIndex + 1);
                    argTypes = (int *)malloc(sizeof(BYTE) * sizeOfArgTypes);
                    memcpy(argTypes, messageBody + lastSeperatorIndex + 1, sizeOfArgTypes);
                    break;
                }
                case 2: {
                    uint32_t sizeOfArgsByte = i - (lastSeperatorIndex + 1);
//                    printf("Expected: %d, Actual: %d\n", argsSize(argTypes), sizeOfArgsByte);
                    assert(sizeOfArgsByte == argsSize(argTypes));
                    argsByte = (BYTE *)malloc(sizeof(BYTE) * sizeOfArgsByte);
                    memcpy(argsByte, messageBody + lastSeperatorIndex + 1, sizeOfArgsByte);
                    break;
                }
                default:
//                    perror("Message Body Error\n");
                    //                    responseType = EXECUTE_FAILURE;
                    serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, -1);
                    free(name);
                    free(argTypes);
                    free(argsByte);
                    free(args);
                    free(messageBody);
                    pthread_exit((void*) 0);;
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    
    printf("Execute Name: %s\n", name);
    //    printf("Execute ArgTypes: ");
    //    for (int i = 0; i < argTypesLength(argTypes); i++) {
    //        printf("%s\n", u32ToBit(argTypes[i]));
    //    }
    //    printf("\n");
    
    //    printOutArgsByte(argTypes, argsByte);
    
    
    // Process for args from argsByte, comsumes (int* argTypes, void** args == NULL,
    // BYTE *argsByte)
    args = (void **)malloc((argTypesLength(argTypes) - 1) * sizeof(void *));
    if (argsByteToArgs(argTypes, argsByte, args)) {
//        printf("args init succeed!\n");
    } else {
        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, -1);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        pthread_exit((void*) 0);;
    }
    
    //    printOutArgs(argTypes, args);
    
    P_NAME_TYPES queryKey(name, argTypes);
    
    // Execute
    skeleton f = serverProcedureToSkeleton.findSkeleton(queryKey);
    if (f == NULL) {
//        printf("591: %s skeleton is null\n", name);
    }
    
    int executionResult = f(argTypes, args);
    if (executionResult < 0) {
        std::cerr << name <<" executes failed!\n";
        // Send EXECUTE_FAILURE
        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, -1);
        // Free allocated varilables
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        pthread_exit((void*) 0);;
    }
//    printf("EXE succeed\n");
    // Send EXECUTE_SUCCESS
    serverResponse(p_args.connectionSocket, EXECUTE_SUCCESS, 0);
    
    //    printOutArgs(argTypes, args);
    
    // Send back execution result
    // Message body: [name,argTypes,argsByte,]
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t sizeOfArgsByte = argsSize(argTypes);
    uint32_t totalSize = sizeOfName + sizeOfArgTypes + sizeOfArgsByte + 3;
    // Send back size should equal to messagelength
    if (p_args.messageLength != totalSize) {
        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, -1);
        pthread_exit((void*) 0);;
    }
    // Prepare messageBody
    BYTE messageBodyResponse[totalSize];
    char seperator = ',';
    int offset = 0;
    memcpy(messageBodyResponse + offset, name, sizeOfName); // [name]
    offset += sizeOfName;
    memcpy(messageBodyResponse + offset, &seperator, sizeof(seperator));//[name,]
    offset += sizeof(seperator);
    memcpy(messageBodyResponse + offset, argTypes, sizeOfArgTypes);// [name, argTypes]
    offset += sizeOfArgTypes;
    memcpy(messageBodyResponse + offset, &seperator, sizeof(seperator));// [name, argTypes,]
    offset += sizeof(seperator);
    for (int i = 0; i < argTypesLength(argTypes) - 1; i++) {
        int lengthOfArray = argTypes[i] & ARG_ARRAY_LENGTH_MASK;
        // If length is 0, scalar
        if (lengthOfArray == 0) {
            lengthOfArray = 1;
        }
        uint32_t copySize = lengthOfArray * argSize(argTypes[i]);
        memcpy(messageBodyResponse + offset, args[i], copySize);
        offset += copySize;
    }
    memcpy(messageBodyResponse + offset, &seperator, sizeof(seperator));
    
    // Send execution result
    ssize_t operationResult = -1;
    operationResult = send(p_args.connectionSocket, &messageBodyResponse, totalSize, 0);
    if (operationResult != totalSize) {
        perror("Server to client: Send [name, argTypes, argsByte,] failed\n");
        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, 0);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        pthread_exit((void*) 0);;
    }
    
    // Free allocated varilables
    if (name != NULL) free(name);
    if (argTypes != NULL) free(argTypes);
    if (argsByte != NULL) free(argsByte);
    if (args != NULL) free(args);
    if (messageBody != NULL) free(messageBody);
    
//    printf("New thread end\n");
    pthread_exit((void*) 0);;
}