//
//  pmap.cc
//  binder
//
//  Created by Honghao on 7/11/14.
//  Copyright (c) 2014 org-honghao. All rights reserved.
//

#include "pmap.h"
#include "rpc.h"
#include "rpc_extend.h"
#include <string>
#include <stdlib.h>
#include <string.h>
#include <iostream>
using namespace std;


#define PROCESS_MASK 0xFFFF0000

P_IP_PORT sentinel(NULL,-1);

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
    if(k1.first == NULL) return false;
    string k1Name(k1.first);
    int *k1Types = k1.second;
    int sizeofk1 = sizeof(int)*argTypesLength(k1Types);
    int *k1copy = (int*)malloc(sizeofk1);
    memcpy(k1copy,k1Types,sizeofk1);
    processArgTypes(k1copy);
    if(k2.first == NULL){ cout<<"k2 first is null"<<endl; return false;}
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

bool pmap::isIpPortEqual(P_IP_PORT k1, P_IP_PORT k2) {
    if(k1.first == NULL || k2.first == NULL){
        return false;
    }
    string k1Ip(k1.first);
    int k1Port = k1.second;
    
    string k2Ip(k2.first);
    int k2Port = k2.second;
    
    if ((k1Ip == k2Ip) && (k1Port == k2Port)) {
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

//#pragma mark - used for rpc_server

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

//#pragma mark - used for rpc_client

P_IP_PORT* pmap::findIp_cached(P_NAME_TYPES key) {
    P_IP_PORT* IpAndPort = NULL;
    
    for(std::vector<P_IP_PORT>::iterator itIp = vecIpPort.begin(); itIp != vecIpPort.end(); itIp++){
        if(itIp->first == NULL) continue;
        for (std::vector<P_MAP_WITHOUTSOCKET>::iterator it = vecIpForCached.begin(); it != vecIpForCached.end(); it++) {
            P_NAME_TYPES existedKey = it->first;
            if (isNameTypesEqual(key, existedKey) && isIpPortEqual(it->second, *itIp)){
                if(IpAndPort == NULL){
                    IpAndPort = (P_IP_PORT*)malloc(sizeof(P_IP_PORT));
                }
                *IpAndPort = it->second;
                P_IP_PORT temp = *itIp;
                vecIpPort.erase(itIp);
                vecIpPort.push_back(temp);
                return IpAndPort;
            }
        }
    }
    
    return IpAndPort;
}

int pmap::insert(P_NAME_TYPES key, P_IP_PORT value){
    
    std::vector<P_MAP_WITHOUTSOCKET>::iterator KVFound = vecIpForCached.end();
    for (std::vector<P_MAP_WITHOUTSOCKET>::iterator it = vecIpForCached.begin(); it != vecIpForCached.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isNameTypesEqual(key, existedKey)) {
            KVFound = it;
            break;
        }
    }
    bool newvalue = true;
    for(std::vector<P_IP_PORT>::iterator it1 = vecIpPort.begin(); it1 != vecIpPort.end(); it1++){
        if(isIpPortEqual(value,*it1)){
            newvalue = false;
            break;
        }
    }
    if(vecIpPort.size() == 0){
        vecIpPort.push_back(sentinel);
        //std::cout<<"pushed sentinel"<<std::endl;
    }
    if(newvalue){
        for(std::vector<P_IP_PORT>::iterator it2 = vecIpPort.begin(); it2 != vecIpPort.end(); it2++){
            if(it2->second == -1){
                vecIpPort.insert(it2, value);
                break;
            }
        }
        //std::cout<<"insert port: "<<value.second<<std::endl;
    }
    
    P_MAP_WITHOUTSOCKET newKV = P_MAP_WITHOUTSOCKET(key, value);
    // Not Found
    if (KVFound == vecIpForCached.end()) {
        vecIpForCached.push_back(newKV);
        return 0;
    }
    // Found
    else {
        //vecIpForCached.erase(KVFound);
        vecIpForCached.push_back(newKV);
        return 1;
    }
}

void pmap::clear_vecIpForCached(P_NAME_TYPES key){
    for (std::vector<P_MAP_WITHOUTSOCKET>::iterator it = vecIpForCached.begin(); it != vecIpForCached.end();) {
            P_NAME_TYPES existedKey = it->first;
            if (isNameTypesEqual(key, existedKey)){
                vecIpForCached.erase(it);
                if(it == vecIpForCached.end()){
                    break;
                }
                continue;
            }
        it++;
    }
}

void pmap::free_P_IP_PORT(P_IP_PORT ip){
    //to free char* in pair
    if(ip.first != NULL){
        delete[] ip.first;
        ip.first = NULL;
    }
}

void pmap::free_P_NAME_TYPES(P_NAME_TYPES name_type){
    if(name_type.first != NULL){
        delete[] name_type.first;
        name_type.first = NULL;
    }
    if(name_type.second != NULL){
        delete[] name_type.second;
        name_type.second = NULL;
    }
}


void pmap::free_P_MAP_SKELETON(P_MAP_SKELETON pair){
    free_P_NAME_TYPES(pair.first);
    
}
void pmap::free_P_MAP_WITHOUTSOCKET(P_MAP_WITHOUTSOCKET pair){
    free_P_NAME_TYPES(pair.first);
    free_P_IP_PORT(pair.second);
}

void pmap::free_P_NAME_TYPES_SOCKET(P_NAME_TYPES_SOCKET pair){
    free_P_NAME_TYPES(pair.first);
}

void pmap::free_P_MAP_IP_PORT(P_MAP_IP_PORT pair){
    free_P_NAME_TYPES_SOCKET(pair.first);
    free_P_IP_PORT(pair.second);
}

pmap::~pmap() {
    
    vecIp.clear();
    vecSkeleton.clear();
}







