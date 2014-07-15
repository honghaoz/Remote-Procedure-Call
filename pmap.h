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

typedef std::pair<char *, int*> P_NAME_TYPES;
typedef std::pair<char *, int> P_IP_PORT;
typedef std::pair<P_NAME_TYPES, P_IP_PORT> P_MAP_IP_PORT;
typedef std::pair<P_NAME_TYPES, skeleton> P_MAP_SKELETON;

class pmap {
    std::vector<P_MAP_IP_PORT> vecIp;
    std::vector<P_MAP_SKELETON> vecSkeleton;
public:
    P_IP_PORT* findIp(P_NAME_TYPES key);
    skeleton findSkeleton(P_NAME_TYPES key);
    int insert(P_NAME_TYPES key, P_IP_PORT value);
    int insert(P_NAME_TYPES key, skeleton value);
//    int remove(P_NAME_TYPES key);
    pmap();
//    pmap(P_NAME_TYPES key, P_IP_PORT value);
    ~pmap();
};

#endif /* defined(__a3_binder__pmap__) */