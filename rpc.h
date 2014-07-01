/*
 * rpc.h
 *
 * This file defines all of the rpc related infomation.
 */
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

#define MSG_SERVER_BINDER_REGISTER 1
#define MSG_CLIENT_BINDER_LOCATE 2
#define MSG_CLIENT_BINDER_TERMINATE 3
#define MSG_CLIENT_SERVER_EXECUTE 4

typedef int (*skeleton)(int *, void **);

// Client side:
extern int rpcCall(char* name, int* argTypes, void** args);
extern int rpcCacheCall(char* name, int* argTypes, void** args);
extern int rpcTerminate();

// Server side:
extern int rpcInit();
extern int rpcRegister(char* name, int* argTypes, skeleton f);
extern int rpcExecute();
    
// Binder side:
extern int rpcBinderInit();
extern int rpcBinderListen();
    
#ifdef __cplusplus
}
#endif

