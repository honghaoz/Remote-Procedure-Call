//
//  pmap.cc
//  a3-binder
//
//  Created by Honghao on 7/11/14.
//  Copyright (c) 2014 org-honghao. All rights reserved.
//

#include "pmap.h"
#include "rpc.h"
#include <string>
using namespace std;


#define PROCESS_MASK 0xFFFF0000

pmap::pmap(){
    // Do nothing
}

void processArgTypes(int* argTypes){
    for (int i = 0; i < argTypesLength(argTypes) - 1; i++) {
        int *eachArgType = &argTypes[i];
        int arrayLenght = *eachArgType & ARG_ARRAY_LENGTH_MASK;
        if (arrayLenght > 0) {
            (*eachArgType) &= PROCESS_MASK;
            (*eachArgType)++;
        }
    }
}


P_IP_PORT *findIP(P_NAME_TYPES key1, int key2){
    
    
}

P_IP_PORT* findIp_client(P_NAME_TYPES key){
    
    
}

bool isNameTypesEqual(P_NAME_TYPES k1, P_NAME_TYPES k2) {
    string k1Name(k1.first);
    int *k1Types = k1.second;
    int sizeofk1 = sizeof(int)*argTypesLength(k1Types);
    int *k1copy = (int*)malloc(sizeofk1);
    memcpy(k1copy,k1Types,sizeofk1);
    std::cout<<"argtypes before process"<<std::endl;
    printOutArgTypes(k1copy);
    processArgTypes(k1copy);
    std::cout<<"argtypes after process"<<std::endl;
    printOutArgTypes(k1copy);
    
    string k2Name(k2.first);
    int *k2Types = k2.second;
    int sizeofk2 = sizeof(int)*argTypesLength(k2Types);
    int *k2copy = (int*)malloc(sizeofk2);
    memcpy(k2copy,k2Types,sizeofk2);
    processArgTypes(k2copy);
    
    if ((k1Name == k2Name) && argTypesEqual(k1Types, k2Types)) {
        return true;
    } else {
        return false;
    }
}

P_IP_PORT* pmap::findIp_client(P_NAME_TYPES key) {
    for (std::vector<P_MAP_IP_PORT>::iterator it = vecIp.begin(); it != vecIp.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isPairEqual(key, existedKey)) {
            return &it->second;
        }
    }
    return NULL;
}

skeleton pmap::findSkeleton(P_NAME_TYPES key) {
    for (std::vector<P_MAP_SKELETON>::iterator it = vecSkeleton.begin(); it != vecSkeleton.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isNameTypesEqual(key, existedKey)) {
            return it->second;
        }
    }
    return NULL;
}

// rpcBinder will call
// Insert new KV, return 0: inserted, return 1: replaced.
int pmap::insert(P_NAME_TYPES_SOCKET key, P_IP_PORT value) {
    std::vector<P_MAP_IP_PORT>::iterator KVFound = vecIp.end();
    for (std::vector<P_MAP_IP_PORT>::iterator it = vecIp.begin(); it != vecIp.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isPairEqual(key, existedKey)) {
            KVFound = it;
        }
    }
    P_MAP_IP_PORT newKV = P_MAP_IP_PORT(key, value);
    // Not Found
    if (KVFound == vecIp.end()) {
        vecIp.push_back(newKV);
        return 0;
    }
    // Found
    else {
        vecIp.erase(KVFound);
        vecIp.push_back(newKV);
        return 1;
    }
}

// rpcServer will call
// Insert new KV, return 0: inserted, return 1: replaced.
int pmap::insert(P_NAME_TYPES key, skeleton value) {
    std::vector<P_MAP_SKELETON>::iterator KVFound = vecSkeleton.end();
    for (std::vector<P_MAP_SKELETON>::iterator it = vecSkeleton.begin(); it != vecSkeleton.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isNameTypesEqual(key, existedKey)) {
            KVFound = it;
        }
    }
    P_MAP_SKELETON newKV = P_MAP_SKELETON(key, value);
    // Not Found
    if (KVFound == vecSkeleton.end()) {
        vecSkeleton.push_back(newKV);
        return 0;
    }
    // Found
    else {
        vecSkeleton.erase(KVFound);
        vecSkeleton.push_back(newKV);
        return 1;
    }
}

pmap::~pmap() {
    vecIp.clear();
    vecSkeleton.clear();
}