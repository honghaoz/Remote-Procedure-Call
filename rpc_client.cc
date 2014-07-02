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

