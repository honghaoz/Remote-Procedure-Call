//
//  pmap.cc
//  a3-binder
//
//  Created by Honghao on 7/11/14.
//  Copyright (c) 2014 org-honghao. All rights reserved.
//

#include "pmap.h"
#include "rpc_helper.cc"
#include <string>
using namespace std;

pmap::pmap(){
    // Do nothing
}

pmap::pmap(P_NAME_TYPES key, P_IP_PORT value) {
    P_MAP newKV = P_MAP(key, value);
    theMap.push_back(newKV);
}

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

P_IP_PORT* pmap::find(P_NAME_TYPES key) {
    for (std::vector<P_MAP>::iterator it = theMap.begin(); it != theMap.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isPairEqual(key, existedKey)) {
            return &it->second;
        }
    }
    return NULL;
}

// Insert new KV, return 0: inserted, return 1: replaced.
int pmap::insert(P_NAME_TYPES key, P_IP_PORT value) {
    std::vector<P_MAP>::iterator KVFound = theMap.end();
    for (std::vector<P_MAP>::iterator it = theMap.begin(); it != theMap.end(); it++) {
        P_NAME_TYPES existedKey = it->first;
        if (isPairEqual(key, existedKey)) {
            KVFound = it;
        }
    }
    P_MAP newKV = P_MAP(key, value);
    // Not Found
    if (KVFound == theMap.end()) {
        theMap.push_back(newKV);
        return 0;
    }
    // Found
    else {
        theMap.erase(KVFound);
        theMap.push_back(newKV);
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
    theMap.clear();
}