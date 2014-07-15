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

pmap::pmap(){
    // Do nothing
}

//pmap::pmap(P_NAME_TYPES key, P_IP_PORT value) {
//    P_MAP_IP_PORT newKV = P_MAP_IP_PORT(key, value);
//    theMap.push_back(newKV);
//}

bool isPairEqual(P_NAME_TYPES k1, P_NAME_TYPES k2) {
    string k1Name(k1.first);
    int *k1Types = k1.second;
    string k2Name(k2.first);
    int *k2Types = k2.second;
    
    if ((k1Name == k2Name) && argTypesEqual(k1Types, k2Types)) {
        return true;
    } else {
        return false;
    }
}

P_IP_PORT* pmap::findIp(P_NAME_TYPES key) {
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
        if (isPairEqual(key, existedKey)) {
            return it->second;
        }
    }
    return NULL;
}

// Insert new KV, return 0: inserted, return 1: replaced.
int pmap::insert(P_NAME_TYPES key, P_IP_PORT value) {
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

// Insert new KV, return 0: inserted, return 1: replaced.
int pmap::insert(P_NAME_TYPES key, skeleton value) {
    std::vector<P_MAP_SKELETON>::iterator KVFound = vecSkeleton.end();
    for (std::vector<P_MAP_SKELETON>::iterator it = vecSkeleton.begin(); it != vecSkeleton.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isPairEqual(key, existedKey)) {
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

//for (std::map<std::pair<char *, int *>, std::pair<char *, int>>::iterator it = binderProcedureToID.begin(); it != binderProcedureToID.end(); it++) {
//    std::pair<char *, int *> key = it->first;
//    std::pair<char *, int> value = it->second;
//    printf("\nKey: \n");
//    printf("Name: %s\n", key.first);
//    printf("ArgTypes: ");
//    for (int i = 0; i < argTypesLength(key.second); i++) {
//        printf("%ud ", key.second[i]);
//    }
//    printf("\n");
//    printf("Value: \n");
//    printf("IP: %s, Port: %d", value.first, value.second);
//    
//}

pmap::~pmap() {
    vecIp.clear();
    vecSkeleton.clear();
}