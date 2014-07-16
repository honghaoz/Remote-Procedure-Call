/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
#ifndef __RPC__H__
#define __RPC__H__

#ifdef __cplusplus
extern "C" {
#endif
 
#define ARG_INPUT   31
#define ARG_OUTPUT  30

#define ARG_CHAR    1
#define ARG_SHORT   2
#define ARG_INT     3
#define ARG_LONG    4
#define ARG_DOUBLE  5
#define ARG_FLOAT   6
    
#define ARG_IO_MASK 0xFF000000
#define ARG_TYPE_MASK 0x00FF0000
#define ARG_ARRAY_LENGTH_MASK 0x0000FFFF

#define ttt long
#define artt ARG_LONG
    
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

#define BYTE uint8_t
typedef int (*skeleton)(int *, void **);

#include <stdint.h>
  
#define INIT_ARGS_I_WITH_TYPE(VAR) \
    if (ArrayLenght == 0) { \
        VAR* var = (VAR*)malloc(sizeof(VAR)); \
        printf("new size: %d\n", sizeof(VAR)); \
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
void printOutArgs(int * &argTypes, void ** &args);
void printOutArgsByte(int * &argTypes, BYTE * &argsByte);
    
// Server side:
extern int rpcInit();
extern int rpcRegister(char* name, int* argTypes, skeleton f);
extern int rpcExecute();
    
// Client side:
extern int rpcCall(char* name, int* argTypes, void** args);
extern int rpcCacheCall(char* name, int* argTypes, void** args);
extern int rpcTerminate();
    
// Binder side:
extern int rpcBinderInit();
extern int rpcBinderListen();
    
#ifdef __cplusplus
}
#endif

#endif
