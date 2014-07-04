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

// Return size of type from an integer in argTypes
uint32_t argSize(uint32_t argInteger) {
    switch ((argInteger & ARG_TYPE_MASK)) {
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