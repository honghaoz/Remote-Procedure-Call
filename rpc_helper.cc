#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <utility>
#include <map>
#include "rpc.h"

//using namespace std;

/******************* Helper Functions ****************
 *
 *  Description: Some handy functions
 *
 ****************************************************/

/**
 *  Convert unsigned 32 bit integer to a bit string
 *
 *  @param x unsigned 32 bit integer
 *
 *  @return string representing in bit
 */
const char *u32ToBit(uint32_t x)
{
    static char b[36]; // includes 32 bits, 3 spaces and 1 trailing zero
    b[0] = '\0';
    
    u_int32_t z;
    int i = 1;
    for (z = 0b10000000000000000000000000000000; z > 0; z >>= 1, i++)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
        if (i % 8 == 0) strcat(b, " "); // seperate 8 bits by a space
    }
    
    return b;
}

void printBitRepresentation(BYTE *x, int byteNumber) {
    char bits[byteNumber * 4 + byteNumber / 4];
    bits[0] = '\0';
    //    printf("%d, %d\n", sizeof(ccc), sizeof(bits));
    int i = 0;
    while (byteNumber > 0) {
        if (byteNumber > 4) {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, 4);
            printf("%s\n", u32ToBit(xx));
            for (z = 0b10000000000000000000000000000000; z > 0; z >>= 1)
            {
                strcat(bits, ((xx & z) == z) ? "1" : "0");
            }
            i++;
            byteNumber -= 4;
            strcat(bits, " ");
            //            printf("'%s'\n", bits);
        }
        else {
            uint32_t z, xx;
            memcpy(&xx, x + 4 * i, byteNumber);
            printf("%s\n", u32ToBit(xx));
            for (z = pow(2, byteNumber * 8 - 1); z > 0; z >>= 1)
            {
                strcat(bits, ((xx & z) == z) ? "1" : "0");
            }
            byteNumber -= byteNumber;
            //            printf("'%s'\n", bits);
        }
    }
    //    printf("%s\n", bits);
}

// Return length of argTypes, includes last 0
uint32_t argTypesLength(int *argTypes) {
    uint32_t i = 0;
    while (argTypes[i] != 0) {
        i++;
    }
    return i + 1;
}

bool argTypesEqual(int *argTypes1, int *argTypes2) {
    int length1 = argTypesLength(argTypes1);
    int length2 = argTypesLength(argTypes2);
    if (length1 == length2) {
        for (int i = 0; i < length1; i++) {
            if (argTypes1[i] != argTypes2[i]) return false;
        }
    } else {
        return false;
    }
    return true;
}

// Return size of type from an integer in argTypes
uint32_t argSize(uint32_t argInteger) {
    int argType = (argInteger & ARG_TYPE_MASK) >> 16;
    switch (argType) {
        case ARG_CHAR:
            return sizeof(char);
            break;
        case ARG_SHORT:
            return sizeof(short);
            break;
        case ARG_INT:
            return sizeof(int);
            break;
        case ARG_LONG:
            return sizeof(long);
            break;
        case ARG_DOUBLE:
            return sizeof(double);
            break;
        case ARG_FLOAT:
            return sizeof(float);
            break;
        default:
            perror("Arg type error!\n");
            return 0;
            break;
    }
}

// Return total size of args
uint32_t argsSize(int *argTypes) {
    uint32_t result = 0;
    uint32_t lengthOfTypes = argTypesLength(argTypes);
    for (int i = 0; i < lengthOfTypes - 1; i++) {
        int lengthOfArray = argTypes[i] & ARG_ARRAY_LENGTH_MASK;
        // If length is 0, scalar
        if (lengthOfArray == 0) {
            lengthOfArray = 1;
        }
        result += lengthOfArray * argSize(argTypes[i]);
    }
    return result;
}

bool argsByteToArgs(int * &argTypes, BYTE * &argsByte, void ** &args) {
    args = (void **)malloc((argTypesLength(argTypes) - 1) * sizeof(void *));
    int offset = 0;
    int argIndex = 0;
    for (int i = 0; i < argTypesLength(argTypes) - 1; i++) {
        uint32_t eachArgType = argTypes[i];
        int argType = (eachArgType & ARG_TYPE_MASK) >> 16;
        switch (argType) {
            case ARG_CHAR: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                INIT_ARGS_I_WITH_TYPE(char)
                break;
            }
            case ARG_SHORT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                INIT_ARGS_I_WITH_TYPE(short)
                break;
            }
            case ARG_INT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
//                printf("length: %d\n", ArrayLenght);
                INIT_ARGS_I_WITH_TYPE(int)
//                args[argIndex] = (void *)&var;
                break;
            }
            case ARG_LONG: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                INIT_ARGS_I_WITH_TYPE(long)
                break;
            }
            case ARG_DOUBLE: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                INIT_ARGS_I_WITH_TYPE(double)
                break;
            }
            case ARG_FLOAT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                INIT_ARGS_I_WITH_TYPE(float)
                break;
            }
            default:
                perror("Arg type error \n");
                return false;
                break;
        }
        argIndex++;
    }
    return true;
}

// print out args
void printOutArgs(int * &argTypes, void ** &args) {
    printf("args: \n");
    for (int i = 0; i < argTypesLength(argTypes) - 1; i++) {
        uint32_t eachArgType = argTypes[i];
        int argType = (eachArgType & ARG_TYPE_MASK) >> 16;
        switch (argType) {
            case ARG_CHAR: {
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    char var = *(char *)args[i];
                    printf(" args[%d] (char) = %c\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(char) * ArrayLenght;
                    char *vars = (char *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (char) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%c, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (char) = array length error\n", i);
                }
                break;
            }
            case ARG_SHORT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    short var = *(short *)args[i];
                    printf(" args[%d] (short) = %hd\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(short) * ArrayLenght;
                    short *vars = (short *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (short) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%hd, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (short) = array length error\n", i);
                }
                break;
            }
            case ARG_INT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    int var = *(int *)args[i];
                    printf(" args[%d] (int) = %d\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(int) * ArrayLenght;
                    int *vars = (int *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (int) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%d, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (int) = array length error\n", i);
                }
                
                break;
            }
            case ARG_LONG: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    long var = *(long *)args[i];
                    printf(" args[%d] (long) = %ld\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(long) * ArrayLenght;
                    long *vars = (long *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (long) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%ld, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (long) = array length error\n", i);
                }
                
                break;
            }
            case ARG_DOUBLE: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    double var = *(double *)args[i];
                    printf(" args[%d] (double) = %f\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(double) * ArrayLenght;
                    double *vars = (double *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (double) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%f, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (double) = array length error\n", i);
                }
                break;
            }
            case ARG_FLOAT: {
                // Get array length
                int ArrayLenght = eachArgType & ARG_ARRAY_LENGTH_MASK;
                if (ArrayLenght == 0) {
                    float var = *(float *)args[i];
                    printf(" args[%d] (float) = %f\n", i, var);
                } else if(ArrayLenght > 0) {
                    uint32_t varsSize = sizeof(float) * ArrayLenght;
                    float *vars = (float *)malloc(varsSize);
                    memcpy(vars, args[i], varsSize);
                    printf(" args[%d] (float) = [", i);
                    for (int varIndex = 0; varIndex < ArrayLenght; varIndex++) {
                        printf("%f, ", vars[varIndex]);
                    }
                    printf("]\n");
                    free(vars);
                } else {
                    printf(" args[%d] (float) = array length error\n", i);
                }
                break;
            }
            default:
                perror("ArgsPrintOut: Arg type error \n");
                break;
        }
    }
    printf("\n");
}