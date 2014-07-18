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
#include "rpc_extend.h"
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

// Used for passing multi arguments to pthread
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
        return status;
    }

    // 2, Create socket connection with Binder
    status = -1;
    status = serverToBinderInit();
    if (status < 0) {
        return status;
    }

    return 0;
}

/**
 *  Init socket for clients and listen it
 *
 *  @return Execution result
 */
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
        fprintf(stderr, "SERVER ERROR: Get addr info for listen socket for clients: %s\n", gai_strerror(status));
        return -31; // ERROR -31;
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
        fprintf(stderr, "SERVER ERROR: Create listen socket for clients failed: %d\n", -32);
//        perror("SERVER ERROR: Create listen socket for clients failed\n");
        return -32; // ERROR -32
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
        fprintf(stderr, "SERVER ERROR: Bind listen socket for clients failed: %d\n", -33);
//        perror("SERVER ERROR: Bind listen socket for clients failed\n");
        return -33; // ERROR -33
    }
    
    // Listen
    listen(serverForClientSocket , MAX_NUMBER_OF_CLIENTS);
    
    // Print out port number
    socklen_t addressLength = sizeof(clientForServer);
    if (getsockname(serverForClientSocket, (struct sockaddr*)&clientForServer, &addressLength) == -1) {
        fprintf(stderr, "SERVER ERROR: Get listen socket for clients port number failed: %d\n", -34);
//        perror("SERVER ERROR: Get listen socket for clients port number failed\n");
        return -34; // ERROR -34
    }
    printf("SERVER_PORT %d\n", ntohs(clientForServer.sin_port));
    portNumber = ntohs(clientForServer.sin_port);
    
    // Since there is only one sock, this is the highest
    serverHighestSocket = serverForClientSocket;
    // Clear the clients
    memset((char *) &serverConnections, 0, sizeof(serverConnections));
    return 0;
}

/**
 *  Init socket to binder and connect it
 *
 *  @return Execution result
 */
int serverToBinderInit() {
    serverToBinderSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverToBinderSocket == -1) {
        fprintf(stderr, "SERVER ERROR: Create socket to binder failed: %d\n", -35);
//        perror("SERVER ERROR: Create socket to binder failed\n");
        return -35; // ERROR -35
    }
    
    struct sockaddr_in serverToBinder;
    // Set address
#warning Need to change to dynamic address
    serverToBinder.sin_addr.s_addr = inet_addr("127.0.0.1");//getenv("BINDER_ADDRESS"));
    serverToBinder.sin_family = AF_INET;
    serverToBinder.sin_port = htons(8888);//htons(atoi(getenv("BINDER_PORT")));
    
    
    // Connect
    if (connect(serverToBinderSocket, (struct sockaddr *)&serverToBinder, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "SERVER ERROR: Connect socket to binder failed: %d\n", -36);
//        perror("SERVER ERROR: Connect socket to binder failed\n");
        return -36; // ERROR -36
    }
    
    // Add serverToBinder socket to connectionList
    if (serverToBinderSocket > serverHighestSocket) {
        serverHighestSocket = serverToBinderSocket;
    }
    serverConnections[0] = serverToBinderSocket;
    
    return 0;
}








#pragma mark - rpcRegister()
int serverHandleRegisterResponse(int connectionSocket);

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
        fprintf(stderr, "SERVER ERROR: Send registration message length error: %d\n", -37);
//        perror("SERVER ERROR: Send registration message length error\n");
        return -37; // ERROR -37
    }
//    printf("Send message length succeed: %zd\n", operationResult);
    
    // Send message type (4 bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageType_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        fprintf(stderr, "SERVER ERROR: Send registration message type error: %d\n", -38);
//        perror("SERVER ERROR: Send registration message type error\n");
        return -38; // ERROR -38
    }
//    printf("Send message type succeed: %zd\n", operationResult);
    
    // Send message body (varied bytes)
    operationResult = -1;
    operationResult = send(serverToBinderSocket, &messageBody, messageLength, 0);
    if (operationResult != messageLength) {
        fprintf(stderr, "SERVER ERROR: Send registration message body error: %d\n", -39);
//        perror("SERVER ERROR: Send registration message body error\n");
        return -39; // ERROR -39
    }
    
    // 3,  Receive registration response from binder
    return serverHandleRegisterResponse(serverToBinderSocket);
}

/**
 *  Server handle register response from binder, either REGISTER_SUCCESS or REGISTER_FAILURE
 *
 *  @param connectionSocket Socket from binder
 *
 *  @return Execution result
 */
int serverHandleRegisterResponse(int connectionSocket) {
    uint32_t responseType_network = 0;
    uint32_t responseType = 0;
    uint32_t responseErrorCode_network = 0;
    int responseErrorCode = 0;
    
    // -10, -11, -12 is reserved for errorCode from binder
    
    // Receive response type
    ssize_t receivedSize = -1;
    receivedSize = recv(connectionSocket, &responseType_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        fprintf(stderr, "SERVER ERROR: Receive registration response type length error: %d\n", -40);
//        perror("SERVER ERROR: Receive registration response type length error\n");
        return -40; // ERROR -40
    }
    receivedSize = -1;
    
    // Receive response error code
    receivedSize = recv(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (receivedSize != sizeof(uint32_t)) {
        fprintf(stderr, "SERVER ERROR: Receive registration response errorCode error: %d\n", -41);
//        perror("SERVER ERROR: Receive registration response errorCode error\n");
        return -41; // ERROR -41
    }
    
    responseType = ntohl(responseType_network);
    responseErrorCode = ntohl(responseErrorCode_network);
    
    if (responseType == REGISTER_FAILURE) {
        //perror("Binder response: REGISTER_FAILURE Error Code: %d\n");
        fprintf(stderr, "SERVER ERROR: Receive REGISTER_FAILURE Error Code: %d\n", responseErrorCode);
//        std::cerr << "SERVER ERROR: Receive REGISTER_FAILURE Error Code: " << responseErrorCode << std::endl;
        return responseErrorCode;
    } else if (responseType == REGISTER_SUCCESS) {
        //        printf("Binder response: REGISTER_SUCCESS\n");
        return responseErrorCode; // May 0 or 1
    } else {
        fprintf(stderr, "SERVER ERROR: Received wrong registration response type: %d\n", -42);
//        perror("SERVER ERROR: Received wrong registration response type\n");
        return -42; // ERROR -42
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
            fprintf(stderr, "SERVER ERROR: Select error: %d\n", -43);
//            perror("SERVER ERROR: Select error\n");
            return -43; // Error -43
        }
        if (numberOfReadSockets > 0) {
            int readResult = serverReadSockets();
            if (readResult == SERVER_TERMINATE) {
                //return -1;
//                printf("Server will terminate\n");
                break;
            } else if (readResult != 0) {
                if (readResult > 0) {
                    printf("SERVER WARNING: %d\n", readResult);
                } else {
                    printf("SERVER ERROR: %d\n", readResult);
                }
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

int serverHandleNewConnection() {
    int newSock;
    struct sockaddr_in client;
    int addr_size = sizeof(struct sockaddr_in);
    
    // Try to accept a new connection
    newSock = accept(serverForClientSocket, (struct sockaddr *)&client, (socklen_t *)&addr_size);
    if (newSock < 0) {
        fprintf(stderr, "SERVER ERROR: Server accpet new client failed: %d\n", -44);
//        perror("SERVER ERROR: Server accpet new client failed\n");
        return -44; // Error -44
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
        fprintf(stderr, "SERVER ERROR: No room left for new client: %d\n", -45);
//        perror("SERVER ERROR: No room left for new client\n");
        close(newSock);
        return -45; // Error -45
    }
    
    return 0;
}

int serverReadSockets() {
    // If listening sock is woked up, there is a new client come in
    if (FD_ISSET(serverForClientSocket, &serverSocketsFD)) {
        int handleResult = serverHandleNewConnection();
        if(handleResult < 0) {
            return handleResult;
        }
    }
    
    // Check whether there is new data come in
    for (int i = 0; i < MAX_NUMBER_OF_CLIENTS; i++) {
        if (FD_ISSET(serverConnections[i], &serverSocketsFD)) {
            int resultOfServerDealWithData = serverDealWithData(i);
            if (resultOfServerDealWithData < 0) {
                return resultOfServerDealWithData;
            } else if (resultOfServerDealWithData == SERVER_TERMINATE) {
                return SERVER_TERMINATE;
            }
        }
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
        fprintf(stderr, "SERVER ERROR: Send response type failed: %d\n", -46);
//        perror("SERVER ERROR: Send response type failed\n");
        return -46; // Error -46
    }
    
    // Send response error code
    operationResult = -1;
    operationResult = send(connectionSocket, &responseErrorCode_network, sizeof(uint32_t), 0);
    if (operationResult != sizeof(uint32_t)) {
        fprintf(stderr, "SERVER ERROR: Send errorCode failed: %d\n", -47);
//        perror("SERVER ERROR: Send errorCode failed\n");
        return -47; // Error -47
    }
//    printf("Server responseType: %d, errorCode: %d\n", responseType, errorCode);
    return errorCode;
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
        fprintf(stderr, "SERVER ERROR: Received wrong message length: %d\n", -48);
//        perror("SERVER ERROR: Received wrong message length\n");
        return serverResponse(connectionSocket, EXECUTE_FAILURE, -48); //Error -48
//        pthread_exit((void*) 0);;
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
        fprintf(stderr, "SERVER ERROR: Received wrong message type length: %d\n", -49);
//        perror("SERVER ERROR: Received wrong message type length\n");
        return serverResponse(connectionSocket, EXECUTE_FAILURE, -49); //Error -49
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
        fprintf(stderr, "SERVER ERROR: Received wrong message type: %d\n", -50);
//        perror("SERVER ERROR: Received wrong message type\n");
        return serverResponse(connectionSocket, EXECUTE_FAILURE, -50); //Error -50
    }
    
    // Allocate messageBody
    BYTE *messageBody = (BYTE *)malloc(sizeof(BYTE) * messageLength);
    
    // Receive message body
    receivedSize = -1;
    receivedSize = recv(connectionSocket, messageBody, messageLength, 0);
    if (receivedSize != messageLength) {
        fprintf(stderr, "SERVER ERROR: Received wrong message body: %d\n", -51);
//        perror("SERVER ERROR: Received wrong message body\n");
        return serverResponse(connectionSocket, EXECUTE_FAILURE, -51); //Error -51
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
        fprintf(stderr, "SERVER ERROR: EXECUTE Create new thread failed: %d\n", -52);
//        perror("SERVER ERROR: EXECUTE Create new thread failed\n");
        return serverResponse(connectionSocket, EXECUTE_FAILURE, -52); //Error -52
//        return -1;
    }
//    printf("Dispatch new execution thread succeed\n");
    return 0;
}

void* serverHandleNewExecution(void *t) {
//    printf("New thread start\n");
    threadArgs p_args = *(threadArgs *)t;
    BYTE *messageBody = p_args.messageBody;
    int connectionSocket = p_args.connectionSocket;
    
    // Message body: [name,argTypes,argsByte,]
    char *name = NULL;
    int *argTypes = NULL;
    BYTE *argsByte = NULL; //expanded args
    void ** args = NULL;
    int lastSeperatorIndex = -1;
    int messageCount = 0;
    
    // Process message body, initialize ip, port, name, argTypes
    for (int i = 0; i < p_args.receivedSize; i++) {
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
                    uint32_t supposedSizeOfArgsByte = argsSize(argTypes);
                    if (supposedSizeOfArgsByte == 0) {
                        if (name != NULL) free(name);
                        if (argTypes != NULL) free(argTypes);
                        if (messageBody != NULL) free(messageBody);
                        fprintf(stderr, "SERVER ERROR: EXECUTE arg type error: %d\n", -101);
//                        perror("SERVER ERROR: EXECUTE argTypes size error\n");
                        serverResponse(connectionSocket, EXECUTE_FAILURE, -101); //Error -101
                        pthread_exit((void*) 0);
                    }
                    if (sizeOfArgsByte != supposedSizeOfArgsByte) {
                        if (name != NULL) free(name);
                        if (argTypes != NULL) free(argTypes);
                        if (messageBody != NULL) free(messageBody);
                        fprintf(stderr, "SERVER ERROR: EXECUTE argTypes size error: %d\n", -53);
//                        perror("SERVER ERROR: EXECUTE argTypes size error\n");
                        serverResponse(connectionSocket, EXECUTE_FAILURE, -53); //Error -53
                        pthread_exit((void*) 0);
                    }
                    argsByte = (BYTE *)malloc(sizeof(BYTE) * sizeOfArgsByte);
                    memcpy(argsByte, messageBody + lastSeperatorIndex + 1, sizeOfArgsByte);
                    break;
                }
                default:
                    if (name != NULL) free(name);
                    if (argTypes != NULL) free(argTypes);
                    if (argsByte != NULL) free(argsByte);
                    if (messageBody != NULL) free(messageBody);
                    fprintf(stderr, "SERVER ERROR: EXECUTE Message body size error: %d\n", -54);
//                    perror("SERVER ERROR: EXECUTE Message body size error\n");
                    serverResponse(connectionSocket, EXECUTE_FAILURE, -54); //Error -54
                    pthread_exit((void*) 0);
                    break;
            }
            lastSeperatorIndex = i;
            messageCount++;
        }
    }
    
    printf("Execute Procedure: %s\n", name);
    //    printf("Execute ArgTypes: ");
    //    for (int i = 0; i < argTypesLength(argTypes); i++) {
    //        printf("%s\n", u32ToBit(argTypes[i]));
    //    }
    //    printf("\n");
    
//    printOutArgsByte(argTypes, argsByte);
    
    
    // Process for args from argsByte, comsumes (int* argTypes, void** args == NULL,
    // BYTE *argsByte)
    args = (void **)malloc((argTypesLength(argTypes) - 1) * sizeof(void *));
    if (!argsByteToArgs(argTypes, argsByte, args)) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        fprintf(stderr, "SERVER ERROR: EXECUTE Init args failed: %d\n", -55);
//        perror("SERVER ERROR: EXECUTE Init args failed\n");
        serverResponse(connectionSocket, EXECUTE_FAILURE, -55); //Error -55
        pthread_exit((void*) 0);;
    }
    
//    printOutArgs(argTypes, args);
    
    P_NAME_TYPES queryKey(name, argTypes);
    
    // Execute
    skeleton f = serverProcedureToSkeleton.findSkeleton(queryKey);
    if (f == NULL) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        fprintf(stderr, "SERVER ERROR: EXECUTE Skeleton not found: %d\n", -56);
//        perror("SERVER ERROR: EXECUTE Skeleton not found\n");
        serverResponse(connectionSocket, EXECUTE_FAILURE, -56); //Error -56
        pthread_exit((void*) 0);
    }
    
    int executionResult = f(argTypes, args);
    if (executionResult < 0) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        fprintf(stderr, "SERVER ERROR: EXECUTE Execute failed: %d\n", -57);
        serverResponse(connectionSocket, EXECUTE_FAILURE, -57); //Error -57
        pthread_exit((void*) 0);
    }
//    printf("EXE succeed\n");
    // Send EXECUTE_SUCCESS
    serverResponse(connectionSocket, EXECUTE_SUCCESS, executionResult);
    
//    printOutArgs(argTypes, args);
    
    // Send back execution result
    // Message body: [name,argTypes,argsByte,]
    uint32_t sizeOfName = (uint32_t)strlen(name) + 1;
    uint32_t sizeOfArgTypes = argTypesLength(argTypes) * sizeof(int);
    uint32_t sizeOfArgsByte = argsSize(argTypes);
    if (sizeOfArgsByte == 0) {
        fprintf(stderr, "SERVER ERROR: EXECUTE arg type error: %d\n", -101);
        //                        perror("SERVER ERROR: EXECUTE argTypes size error\n");
//        serverResponse(connectionSocket, EXECUTE_FAILURE, -101); //Error -101
        pthread_exit((void*) 0);
    }
    uint32_t totalSize = sizeOfName + sizeOfArgTypes + sizeOfArgsByte + 3;
    // Send back size should equal to messagelength
    if (p_args.messageLength != totalSize) {
//        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, -1);
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        fprintf(stderr, "SERVER ERROR: EXECUTE Send back execution message length failed: %d\n", -58);
//        perror("SERVER ERROR: EXECUTE Send back execution message length failed\n");
        pthread_exit((void*) 0);
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
        uint32_t eachArgSize = argSize(argTypes[i]);
        if (!(eachArgSize > 0)) {
            if (name != NULL) free(name);
            if (argTypes != NULL) free(argTypes);
            if (argsByte != NULL) free(argsByte);
            if (args != NULL) free(args);
            if (messageBody != NULL) free(messageBody);
            fprintf(stderr, "SERVER ERROR: EXECUTE Send back execution message, wrong argType failed: %d\n", -101);
//            perror("SERVER ERROR: EXECUTE Send back execution message, wrong argType failed\n");
            pthread_exit((void*) 0);
        }
        uint32_t copySize = lengthOfArray * eachArgSize;
        memcpy(messageBodyResponse + offset, args[i], copySize);
        offset += copySize;
    }
    memcpy(messageBodyResponse + offset, &seperator, sizeof(seperator));
    
    // Send execution result
    ssize_t operationResult = -1;
    operationResult = send(p_args.connectionSocket, &messageBodyResponse, totalSize, 0);
    if (operationResult != totalSize) {
        if (name != NULL) free(name);
        if (argTypes != NULL) free(argTypes);
        if (argsByte != NULL) free(argsByte);
        if (args != NULL) free(args);
        if (messageBody != NULL) free(messageBody);
        fprintf(stderr, "SERVER ERROR: EXECUTE Send back execution result failed: %d\n", -59);
//        perror("SERVER ERROR: EXECUTE Send back execution result failed\n");
        pthread_exit((void*) 0);
        
//        perror("Server to client: Send [name, argTypes, argsByte,] failed\n");
//        serverResponse(p_args.connectionSocket, EXECUTE_FAILURE, 0);
//        if (name != NULL) free(name);
//        if (argTypes != NULL) free(argTypes);
//        if (argsByte != NULL) free(argsByte);
//        if (args != NULL) free(args);
//        if (messageBody != NULL) free(messageBody);
//        pthread_exit((void*) 0);;
    }
    
    // Free allocated varilables
    if (name != NULL) free(name);
    if (argTypes != NULL) free(argTypes);
    if (argsByte != NULL) free(argsByte);
    if (args != NULL) free(args);
    if (messageBody != NULL) free(messageBody);
    
//    printf("New thread end\n");
    pthread_exit((void*) 0);
}