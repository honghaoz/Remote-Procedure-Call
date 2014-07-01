/************************
 CS454 A3 binder.cc
 
 created by Chao

*************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "rpc.h"

int main()
{
    int status = -1;
    status = rpcBinderInit();
    if (status < 0) {
        exit(EXIT_FAILURE);
    }
    rpcBinderListen();
    return 0;
}