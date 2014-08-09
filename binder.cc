/************************
 
 created by Chao

*************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "rpc.h"
#include "rpc_extend.h"

int main()
{
    int status = -1;
    status = rpcBinderInit();
    if (status < 0) {
        exit(status);
    }
    status = -1;
    status = rpcBinderListen();
    exit(status);
}