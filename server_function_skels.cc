#include "server_functions.h"
#include <stdio.h>
#include <string.h>

//int f0_Skel(int *argTypes, void **args) {
////    int a0 = *(int *)args[1];
////    int b0 = *(int *)args[2];
////    int return0 = 0;
//  *(int *)args[0] = f0(*(int *)args[1], *(int *)args[2]);
////    return0 = *(int *)args[0];
////    printf("%d + %d = %d\n", a0, b0, return0);
//  return 0;
//}

int f0_Skel(int *argTypes, void **args) {
    //    int a0 = *(int *)args[1];
    //    int b0 = *(int *)args[2];
    //    int return0 = 0;
    *(char *)args[0] = f0(*(char *)args[1], *(char *)args[2]);
    //    return0 = *(int *)args[0];
    //    printf("%d + %d = %d\n", a0, b0, return0);
    return 0;
}

int f1_Skel(int *argTypes, void **args) {

  *((long *)*args) = f1( *((char *)(*(args + 1))), 
		        *((short *)(*(args + 2))),
		        *((int *)(*(args + 3))),
		        *((long *)(*(args + 4))) );

  return 0;
}

int f2_Skel(int *argTypes, void **args) {

  /* (char *)*args = f2( *((float *)(*(args + 1))), *((double *)(*(args + 2))) ); */
  *args = f2( *((float *)(*(args + 1))), *((double *)(*(args + 2))) );

  return 0;
}

int f3_Skel(int *argTypes, void **args) {

  f3((long *)(*args));
  return 0;
}

/* 
 * this skeleton doesn't do anything except returns
 * a negative value to mimic an error during the 
 * server function execution, i.e. file not exist
 */
int f4_Skel(int *argTypes, void **args) {

  return -1; /* can not print the file */
}

