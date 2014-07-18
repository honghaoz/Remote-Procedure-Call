/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
#ifndef __RPC_EXTEND__H__
#define __RPC_EXTEND__H__

 
    
#define ARG_IO_MASK 0xFF000000
#define ARG_TYPE_MASK 0x00FF0000
#define ARG_ARRAY_LENGTH_MASK 0x0000FFFF
    
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
    TERMINATE = 10,
    LOC_CACHED_REQUEST = 11,
    LOC_CACHED_SUCCESS,
    LOC_CACHED_FAILURE
};

#define BYTE uint8_t

#include <stdint.h>
  
#define INIT_ARGS_I_WITH_TYPE(VAR) \
int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK; \
    if (ArrayLenght == 0) { \
        VAR* var = (VAR*)malloc(sizeof(VAR)); \
        memcpy(var, argsByte + offset, sizeof(VAR)); \
        args[argIndex] = (void *)var; \
        offset += sizeof(VAR); \
    } else if (ArrayLenght > 0) { \
        uint32_t varsSize = sizeof(VAR) * ArrayLenght; \
        VAR *vars = (VAR *)malloc(varsSize); \
        memcpy(vars, argsByte + offset, varsSize); \
        args[argIndex] = (void *)vars; \
        offset += varsSize; \
    } else { \
        perror("ARG_LENGTH error\n"); \
    }

// Helper functions:
const char *u32ToBit(uint32_t x);
void printBitRepresentation(BYTE *x, int byteNumber);
uint32_t argTypesLength(int *argTypes);
uint32_t argSize(uint32_t argInteger);
uint32_t argsSize(int *argTypes);
bool argTypesEqual(int *argTypes1, int *argTypes2);
bool argsByteToArgs(int * &argTypes, BYTE * &argsByte, void ** &args);
void printOutArgTypes(int *argTypes);
void printOutArgs(int * &argTypes, void ** &args);
void printOutArgsByte(int * &argTypes, BYTE * &argsByte);

extern int rpcBinderInit();
extern int rpcBinderListen();
    



#endif
