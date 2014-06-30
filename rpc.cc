#include "rpc.h"
#include <iostream>
using namespace std;

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