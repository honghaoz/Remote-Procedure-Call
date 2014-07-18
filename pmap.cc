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
#include <stdlib.h>
#include <string.h>
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



bool isNameTypesEqual(P_NAME_TYPES k1, P_NAME_TYPES k2) {
    string k1Name(k1.first);
    int *k1Types = k1.second;
    int sizeofk1 = sizeof(int)*argTypesLength(k1Types);
    int *k1copy = (int*)malloc(sizeofk1);
    memcpy(k1copy,k1Types,sizeofk1);
//    std::cout<<"argtypes before process"<<std::endl;
//    printOutArgTypes(k1copy);
    processArgTypes(k1copy);
//    std::cout<<"argtypes after process"<<std::endl;
//    printOutArgTypes(k1copy);
    
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

bool isNameTypesSocketEqual(P_NAME_TYPES_SOCKET k1, P_NAME_TYPES_SOCKET k2) {
    P_NAME_TYPES nameTypesKey1 = k1.first;
    int socketKey1 = k1.second;
    P_NAME_TYPES nameTypesKey2 = k2.first;
    int socketKey2 = k2.second;
    if (isNameTypesEqual(nameTypesKey1, nameTypesKey2) && (socketKey1 == socketKey2)) {
        return true;
    } else {
        return false;
    }
}


P_IP_PORT* pmap::findIP(P_NAME_TYPES key1, int key2){
    P_NAME_TYPES_SOCKET key(key1,key2); // create the key to find ip and port
    for(std::vector<P_MAP_IP_PORT>::iterator it = vecIp.begin(); it != vecIp.end();it++){
        if(isNameTypesSocketEqual(it->first,key)){
            return &(it->second); // return the poiner of ip&port pair
        }
    }
    return NULL;
}


P_IP_PORT* pmap::findIp_client(P_NAME_TYPES key) {
    P_IP_PORT* IpAndPort = NULL;
    for (std::vector<int>::iterator it = vecSocket.begin(); it != vecSocket.end(); it++) {
        IpAndPort = this->findIP(key, *it);
        if(IpAndPort != NULL){// if get the value, put the socket to the bottom of vector
            int socket = *it; 
            this->vecSocket.erase(it);
            this->vecSocket.push_back(socket);
            break;
        }
    }
    return IpAndPort;
}

/**
 *  Get a list of ip/port that have registered with key (name, types)
 *
 *  @param key P_NAME_TYPES, procedure to be found
 *
 *  @return vector<P_IP_PORT>, a list of found ip/ports
 */
std::vector<P_IP_PORT> pmap::findIpList_client(P_NAME_TYPES key) {
    std::vector<P_IP_PORT> ipPorts;
    for (std::vector<P_MAP_IP_PORT>::iterator it = vecIp.begin(); it != vecIp.end(); it++) {
        // Get each name type
        P_NAME_TYPES eachNameTypes = it->first.first;
        // Get the ip/port for each name type
        P_IP_PORT eachIpPort = it->second;
        // If queried key is equal to each name type(key), add to ipPorts
        if (isNameTypesEqual(eachNameTypes, key)) {
            ipPorts.push_back(eachIpPort);
        }
    }
    return ipPorts;
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

/**
 *  Try to add newSocket to Socket list (vecSocket)
 *  If newSocket is already exsit in vecSocket, do nothing
 *  Else, append newSocket to vecSocket
 *
 *  @param newSocket Socket file descriptor
 */
void pmap::addToVectorSocket(int newSocket) {
    std::vector<int>::iterator socketFound = vecSocket.end();
    for (std::vector<int>::iterator it = vecSocket.begin(); it != vecSocket.end(); it++) {
        // If newSocket is equal to current socket
        if (newSocket == *it) {
            socketFound = it;
            break;
        }
    }
    // Not found, add newSocket to vecSocket
    if (socketFound == vecSocket.end()) {
        vecSocket.push_back(newSocket);
    }
}

/**
 *  Binder register new procedure with it's IP/Port
 *  rpcBinder will call.
 *
 *  @param key   P_NAME_TYPES_SOCKET, new procedure to be registered
 *  @param value P_IP_PORT, new IP/Port to be registered
 *
 *  @return 0, inserted. 1, replaced.
 */
int pmap::insert(P_NAME_TYPES_SOCKET key, P_IP_PORT value) {
    
    // Figure out whether new key is exist
    std::vector<P_MAP_IP_PORT>::iterator KVFound = vecIp.end();
    for (std::vector<P_MAP_IP_PORT>::iterator it = vecIp.begin(); it != vecIp.end(); it++) {
        P_NAME_TYPES_SOCKET existedKey = it->first;
        if (isNameTypesSocketEqual(key, existedKey)) {
            KVFound = it;
            break;
        }
    }
    P_MAP_IP_PORT newKV = P_MAP_IP_PORT(key, value);
    // Not Found, add new entry to vecIp and try to add socket to vecSocket
    if (KVFound == vecIp.end()) {
        vecIp.push_back(newKV);
        int newSocket = key.second;
        addToVectorSocket(newSocket);
        return 0;
    }
    // Found, replace old entry with new entr. No need to add to vecSocket
    else {
        vecIp.erase(KVFound);
        vecIp.push_back(newKV);
        return 1;
    }
}

/**
 *  Server register new procedure with this procedure's skeleton
 *  rpcServer will call.
 *
 *  @param key   P_NAME_TYPES, new procedure to be registered
 *  @param value skeleton, skeleton for the new procedure
 *
 *  @return 0, inserted. 1, replaced.
 */
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