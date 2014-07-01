#include "rpc.h"
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
using namespace std;

// Server side:
int rpcInit() {
    cout << "rpcInit()" << endl;
    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    cout << "rpcRegister(" << name << ")" << endl;
    return 0;
}
int rpcExecute() {
    cout << "rpcExecute()" << endl;
    return 0;
}

// Client side:
int rpcCall(char* name, int* argTypes, void** args) {
    cout << "rpcCall(" << name << ")" << endl;
    return 0;
}
int rpcCacheCall(char* name, int* argTypes, void** args) {
    cout << "rpcCacheCall(" << name << ")" << endl;
    return 0;
}
int rpcTerminate() {
    cout << "rpcTerminate()" << endl;
    return 0;
}

// Binder side:
int rpcBinderInit() {
    cout << "rpcBinderInit()" << endl;
    
    
    
    return 0;
}