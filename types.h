/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
#ifndef __TYPES__H__
#define __TYPES__H__

enum messageType{
    REGISTER = 1,
    REGISTER_SUCCESS = 2,
    REGISTER_FAILURE = 3,
    LOC_REQUEST = 4,
    LOC_SUCCESS = 5,
    LOC_FAILURE = 6,
    EXECUTE = 7,
    EXECUTE_SUCCESS = 8,
    EXECUTE_FAILURE = 9,
    TERMINATE = 10
};




#endif
