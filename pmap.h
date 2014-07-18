//
//  pmap.h
//  a3-binder
//
//  Created by Honghao on 7/11/14.
//  Copyright (c) 2014 org-honghao. All rights reserved.
//

#ifndef __a3_binder__pmap__
#define __a3_binder__pmap__

#include <iostream>
#include <vector>
#include <typeinfo>
#include "rpc.h"

// Keys
typedef std::pair<char *, int*> P_NAME_TYPES;
typedef std::pair<P_NAME_TYPES, int> P_NAME_TYPES_SOCKET;

// Values
typedef std::pair<char *, int> P_IP_PORT;
typedef std::pair<P_NAME_TYPES, P_IP_PORT> P_MAP_WITHOUTSOCKET;
typedef std::pair<P_NAME_TYPES_SOCKET, P_IP_PORT> P_MAP_IP_PORT;
typedef std::pair<P_NAME_TYPES, skeleton> P_MAP_SKELETON;

class pmap {
    std::vector<P_MAP_IP_PORT> vecIp;
    std::vector<P_MAP_SKELETON> vecSkeleton;
    std::vector<P_MAP_WITHOUTSOCKET> vecIpForCached;
    std::vector<int> vecSocket;
    std::vector<P_IP_PORT> vecIpPort;
    
    void addToVectorSocket(int newSocket);
    P_IP_PORT* findIP(P_NAME_TYPES key1, int key2);
public:
    P_IP_PORT* findIp_client(P_NAME_TYPES key); // rpcBinder will call
    P_IP_PORT* findIp_cached(P_NAME_TYPES key); // rpcClient will call

    std::vector<P_IP_PORT> findIpList_client(P_NAME_TYPES key); // rpcBinder will call (cached call)
    skeleton findSkeleton(P_NAME_TYPES key); // rpcServer will call
    int insert(P_NAME_TYPES_SOCKET key, P_IP_PORT value); // rpcBinder register
    int insert(P_NAME_TYPES key, skeleton value); //  rpcServer register
    int insert(P_NAME_TYPES key, P_IP_PORT value);
    void clear_vecIpForCached(P_NAME_TYPES key, P_IP_PORT value);
    pmap();
    ~pmap();
};

#endif /* defined(__a3_binder__pmap__) */
